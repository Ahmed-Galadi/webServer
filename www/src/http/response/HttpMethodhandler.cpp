#include "HttpMethodHandler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include "Response.hpp"
#include "../../config/ServerConfig.hpp"
#include "../../config/ParseUtils.hpp"
#include "../httpMethods/post/POSThandler.hpp"
#include "../httpMethods/get/GEThandler.hpp"
#include "../httpMethods/delete/DELETEhandler.hpp"
#include "../httpMethods/cgi/CGIhandler.hpp"

// ============================================
// Helper function to check if URI is an upload/API endpoint
// ============================================
static bool isUploadEndpoint(const std::string& uri, const LocationConfig* location) {
    std::string locationPath = location->getPath();
    
    // Exact match with location path = it's an endpoint
    // Example: /upload-image matches location /upload-image
    if (uri == locationPath) {
        return true;
    }
    
    // URI without trailing slash matches location with trailing slash
    // Example: /upload matches location /upload/
    if (locationPath.length() > 1 && 
        locationPath[locationPath.length() - 1] == '/' &&
        uri == locationPath.substr(0, locationPath.length() - 1)) {
        return true;
    }
    
    // Location without trailing slash matches URI with trailing slash
    // Example: /upload/ matches location /upload
    if (uri.length() > 1 && 
        uri[uri.length() - 1] == '/' &&
        locationPath == uri.substr(0, uri.length() - 1)) {
        return true;
    }
    
    return false;
}

static bool hasFileExtension(const std::string& uri) {
    size_t lastDot = uri.find_last_of('.');
    size_t lastSlash = uri.find_last_of('/');
    
    // Has extension after last slash
    return (lastDot != std::string::npos && 
            (lastSlash == std::string::npos || lastDot > lastSlash));
}

// ============================================
// Helper Functions
// ============================================

static bool isDirectory(const std::string &path) {
    struct stat s;
    if (stat(path.c_str(), &s) == 0) {
        return S_ISDIR(s.st_mode);
    }
    return false;
}

static bool fileExists(const std::string &path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

static std::string resolveFilePath(const std::string& uri, 
                                   const LocationConfig* location,
                                   const ServerConfig& serverConfig) {
    // Get root from location, fallback to server root
    std::string root = location->getRoot();
    if (root.empty()) {
        root = serverConfig.getRoot();
    }
    
    // Remove location path prefix from URI
    std::string locationPath = location->getPath();
    std::string relativePath = uri;
    
    if (uri.find(locationPath) == 0) {
        relativePath = uri.substr(locationPath.length());
    }
    
    // Build full path
    std::string fullPath = root;
    if (!relativePath.empty() && relativePath != "/" && relativePath[0] != '/') {
        fullPath += "/";
    }
    fullPath += relativePath;
    
    // DON'T append index here - let the method handler decide
    // This allows proper 403 handling when directory exists but index doesn't
    if (isDirectory(fullPath)) {
        if (fullPath[fullPath.length() - 1] != '/') {
            fullPath += "/";
        }
    }
    
    return fullPath;
}

static bool isCgiByExtension(const std::string& uri) {
    // Strip query string first
    std::string uriPath = uri;
    size_t queryPos = uriPath.find('?');
    if (queryPos != std::string::npos) {
        uriPath = uriPath.substr(0, queryPos);
    }
    
    const char* extensions[] = {".php", ".py", ".js", ".rb", ".pl", ".cgi", NULL};
    
    for (int i = 0; extensions[i] != NULL; ++i) {
        std::string ext = extensions[i];
        if (uriPath.length() >= ext.length()) {
            if (uriPath.substr(uriPath.length() - ext.length()) == ext) {
                return true;
            }
        }
    }
    return false;
}

// ============================================
// HTTP Method Dispatcher - Configuration-Based
// ============================================

Response* HttpMethodDispatcher::executeHttpMethod(const Request &request, 
                                                   const ServerConfig &serverConfig) {
    std::string method = request.getMethod();
    std::string uri = request.getURI();
    
    
    // ============================================
    // STEP 1: Find matching location from config
    // ============================================
    const LocationConfig* location = serverConfig.findLocation(uri);
    
    if (!location) {
        return Response::makeErrorResponse(404, &serverConfig);
    }
    
    
    // ============================================
    // STEP 1.5: Check if location has a return directive
    // ============================================
    if (location->hasReturn()) {
        int returnCode = location->getReturnCode();
        std::string returnUrl = location->getReturnUrl();
        
        
        Response* response = new Response();
        response->setStatus(returnCode);
        response->setVersion("HTTP/1.0");
        
        // Set appropriate reason phrase
        if (returnCode == 301)
            response->setReasonPhrase("Moved Permanently");
        else if (returnCode == 302)
            response->setReasonPhrase("Found");
        else if (returnCode == 307)
            response->setReasonPhrase("Temporary Redirect");
        
        response->setServer("WebServer/1.0");
        response->setDate();
        response->setConnection("close");
        
        std::map<std::string, std::string> headers;
        headers["Location"] = returnUrl;
        response->setHeaders(headers);
        
        std::ostringstream body;
        body << "<html><body><h1>" << returnCode << " Redirect</h1>"
             << "<p>Redirecting to <a href=\"" << returnUrl << "\">" << returnUrl << "</a></p>"
             << "</body></html>";
        response->setBody(body.str());
        
        return response;
    }
    
    // ============================================
    // STEP 2: Check if method is allowed for this location
    // ============================================
    if (!location->isMethodAllowed(method))
    {
        Response* response = Response::makeErrorResponse(405, &serverConfig);
        
        // Build Allow header with allowed methods from config
        std::vector<std::string> allowedMethods = location->getMethods();
        std::string allowHeader = "";
        for (size_t i = 0; i < allowedMethods.size(); ++i) {
            if (i > 0) allowHeader += ", ";
            allowHeader += allowedMethods[i];
        }
        
        response->setVersion("HTTP/1.0");
        std::map<std::string, std::string> headers = response->getHeaders();
        headers["Allow"] = allowHeader;
        response->setHeaders(headers);
        
        return response;
    }

    if (location->isCGIEnabled())
    {
        if (isCgiByExtension(uri))
        {
            HttpMethodHandler *handler = new CGIhandler();
            Response *output = handler->handler(request, location, &serverConfig);
            delete handler;
            return output;
        }
    }
    // exit(1);
    // ============================================
    // STEP 3: Resolve resource path and check existence
    // ============================================
    std::string filePath = resolveFilePath(uri, location, serverConfig);
    bool resourceExists = fileExists(filePath) || isDirectory(filePath);
    
    
    // ============================================
    // STEP 4: Create appropriate handler based on method
    // ============================================
    HttpMethodHandler *handler = NULL;
    
    if (method == "GET") {
        // GET: Resource must exist
        if (!resourceExists && !location->getAutoIndex()) {
            return Response::makeErrorResponse(404, &serverConfig);
        }
        handler = new GEThandler();
    }
    else if (method == "POST") {
        // POST: Flexible handling for different scenarios
        // - Upload endpoints (/upload, /upload-image, etc.) → Allow
        // - API endpoints (matching location path) → Allow
        // - Regular files with extensions → Must exist
        
        bool isEndpoint = isUploadEndpoint(uri, location);
        bool hasExtension = hasFileExtension(uri);
        
        // Only require file existence if it's not an endpoint and has an extension
        if (!isEndpoint && hasExtension && !resourceExists) {
            return Response::makeErrorResponse(404, &serverConfig);
        }
        
        // All other cases: let POST handler deal with it
        handler = new POSThandler();
    }
    else if (method == "DELETE") {
        // DELETE: Resource must exist (can't delete non-existent file)
        if (!resourceExists) {
            return Response::makeErrorResponse(404, &serverConfig);
        }
        handler = new DELETEhandler();
    }
    else {
        // Method recognized but not implemented
        
        Response* response = Response::makeErrorResponse(501, &serverConfig);
        
        // Build Allow header from location config
        std::vector<std::string> allowedMethods = location->getMethods();
        std::string allowHeader = "";
        for (size_t i = 0; i < allowedMethods.size(); ++i) {
            if (i > 0) allowHeader += ", ";
            allowHeader += allowedMethods[i];
        }
        
        std::map<std::string, std::string> headers = response->getHeaders();
        headers["Allow"] = allowHeader;
        response->setHeaders(headers);
        
        return response;
    }
    
    // ============================================
    // STEP 4: Execute handler with location context
    // ============================================
    Response *output = NULL;
    try {
        // Pass both request and location config to handler
        output = handler->handler(request, location, &serverConfig);
        
    } catch (const Request::ForbiddenMethod& e) {
        delete handler;
        
        Response* response = Response::makeErrorResponse(405, &serverConfig);
        
        // Build Allow header from location
        std::vector<std::string> allowedMethods = location->getMethods();
        std::string allowHeader = "";
        for (size_t i = 0; i < allowedMethods.size(); ++i) {
            if (i > 0) allowHeader += ", ";
            allowHeader += allowedMethods[i];
        }
        
        std::map<std::string, std::string> headers = response->getHeaders();
        headers["Allow"] = allowHeader;
        response->setHeaders(headers);
        
        return response;
        
    } catch (const Request::InvalidRequest& e) {
        delete handler;
        return Response::makeErrorResponse(400, &serverConfig);
        
    } catch (const std::exception& e) {
        delete handler;
        return Response::makeErrorResponse(500, &serverConfig);
    }
    
    delete handler;
    return output;
}

// Virtual destructor implementation
HttpMethodHandler::~HttpMethodHandler() {
}