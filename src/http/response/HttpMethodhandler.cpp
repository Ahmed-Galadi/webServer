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

static bool isUploadEndpoint(const std::string& uri, const LocationConfig* location) {
    std::string locationPath = location->getPath();
    
    if (uri == locationPath) {
        return true;
    }
    
    if (locationPath.length() > 1 && 
        locationPath[locationPath.length() - 1] == '/' &&
        uri == locationPath.substr(0, locationPath.length() - 1)) {
        return true;
    }
    
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
    
    return (lastDot != std::string::npos && 
            (lastSlash == std::string::npos || lastDot > lastSlash));
}

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
    std::string root = location->getRoot();
    if (root.empty()) {
        root = serverConfig.getRoot();
    }
    
    std::string locationPath = location->getPath();
    std::string relativePath = uri;
    
    if (uri.find(locationPath) == 0) {
        relativePath = uri.substr(locationPath.length());
    }
    
    std::string fullPath = root;
    if (!relativePath.empty() && relativePath != "/" && relativePath[0] != '/') {
        fullPath += "/";
    }
    fullPath += relativePath;
    
    if (isDirectory(fullPath)) {
        if (fullPath[fullPath.length() - 1] != '/') {
            fullPath += "/";
        }
    }
    
    return fullPath;
}

static bool isCgiByExtension(const std::string& uri) {
    std::string uriPath = uri;
    size_t queryPos = uriPath.find('?');
    if (queryPos != std::string::npos) {
        uriPath = uriPath.substr(0, queryPos);
    }
    
    const char* extensions[] = {".py", ".js", NULL};
    
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

Response* HttpMethodDispatcher::executeHttpMethod(const Request &request, 
                                                   const ServerConfig &serverConfig) {
    std::string method = request.getMethod();
    std::string uri = request.getURI();
    
    
    const LocationConfig* location = serverConfig.findLocation(uri);
    
    if (!location) {
        return Response::makeErrorResponse(404, &serverConfig);
    }
    
    
    if (location->hasReturn()) {
        int returnCode = location->getReturnCode();
        std::string returnUrl = location->getReturnUrl();
        
        
        Response* response = new Response();
        response->setStatus(returnCode);
        response->setVersion("HTTP/1.0");
        
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
    
    if (!location->isMethodAllowed(method))
    {
        Response* response = Response::makeErrorResponse(405, &serverConfig);
        
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
    std::string filePath = resolveFilePath(uri, location, serverConfig);
    bool resourceExists = fileExists(filePath) || isDirectory(filePath);
    
    
    HttpMethodHandler *handler = NULL;
    
    if (method == "GET") {
        if (!resourceExists && !location->getAutoIndex()) {
            return Response::makeErrorResponse(404, &serverConfig);
        }
        handler = new GEThandler();
    }
    else if (method == "POST") {
        
        bool isEndpoint = isUploadEndpoint(uri, location);
        bool hasExtension = hasFileExtension(uri);
        
        if (!isEndpoint && hasExtension && !resourceExists) {
            return Response::makeErrorResponse(404, &serverConfig);
        }
        
        handler = new POSThandler();
    }
    else if (method == "DELETE") {
        if (!resourceExists) {
            return Response::makeErrorResponse(404, &serverConfig);
        }
        handler = new DELETEhandler();
    }
    else {
        
        Response* response = Response::makeErrorResponse(501, &serverConfig);
        
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
    
    Response *output = NULL;
    try {
        output = handler->handler(request, location, &serverConfig);
        
    } catch (const Request::ForbiddenMethod& e) {
        delete handler;
        
        Response* response = Response::makeErrorResponse(405, &serverConfig);
        
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

HttpMethodHandler::~HttpMethodHandler() {
}