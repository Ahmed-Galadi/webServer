#include "./POSThandler.hpp"
#include "../../requestParse/RequestParser.hpp"
#include <sstream>
#include <cctype>
#include <cstddef>
#include "../../../../include/GlobalUtils.hpp"

Response* POSThandler::handler(const Request& req, const LocationConfig* location, const ServerConfig* /* serverConfig */) {
    Response* response = new Response();
    
    try {
        // Validate Content-Length (required in HTTP/1.0)
        std::map<std::string, std::string> headers = req.getHeaders();
        bool hasContentLength = false;
        
        for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
             it != headers.end(); ++it) {
            if (toLowerCase(it->first) == "content-length") {
                hasContentLength = true;
                break;
            }
        }
        
        if (!hasContentLength) {
            delete response;
            return createErrorResponse(411, "Length Required");
        }
        
        // Get Content-Type
        std::string contentType = getContentType(headers);
        if (contentType.empty()) {
            delete response;
            return createErrorResponse(400, "Bad Request - Missing Content-Type header");
        }
        
        // Check if Content-Type is supported
        bool isSupportedType = (
            contentType.find("multipart/form-data") != std::string::npos ||
            contentType.find("application/x-www-form-urlencoded") != std::string::npos ||
            contentType.find("application/json") != std::string::npos ||
            contentType.find("text/plain") != std::string::npos ||
            contentType.find("application/octet-stream") != std::string::npos
        );
        
        if (!isSupportedType) {
            delete response;
            return createErrorResponse(415, "Unsupported Media Type");
        }
        
        // Set response properties
        response->setVersion(req.getVersion());
        response->setServer("WebServer/1.0");
        response->setVersion("HTTP/1.0");
        response->setDate();
        response->setConnection("close");
        
        std::string uri = req.getURI();
        std::vector<RequestBody> bodyParts = RequestParser::ParseBody(req);
        
        // Route to appropriate handler
        if (contentType.find("multipart/form-data") != std::string::npos) {
            // ← UPDATED: Pass location to multipart handler
            return handleMultipartFormData(req, response, bodyParts, uri, location);
        }
        else if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
            return handleFormUrlEncoded(req, response, bodyParts, uri);
        }
        else if (contentType.find("application/json") != std::string::npos) {
            return handleJsonData(req, response, uri);
        }
        else if (contentType.find("text/plain") != std::string::npos) {
            return handlePlainText(req, response, uri);
        }
        else if (contentType.find("application/octet-stream") != std::string::npos) {
            return handleBinaryData(req, response, bodyParts, uri);
        }
        else {
            return handleDefaultPost(req, response, uri);
        }
        
    } catch (const std::exception& e) {
        delete response;
        return createErrorResponse(500, "Internal Server Error");
    }
}

// ← NEW: Get upload directory from location config
std::string POSThandler::getUploadDirectory(const LocationConfig* location) const {
    if (location) {
        // Priority 1: Check if upload_store directive is set
        std::string uploadStore = location->getUploadStore();
        if (!uploadStore.empty()) {
            return uploadStore;
        }
        
        // Priority 2: Fallback to root directive
        std::string root = location->getRoot();
        if (!root.empty()) {
            return root;
        }
    }
    
    // Priority 3: Default fallback
    return "./www/upload";
}



std::string POSThandler::getContentType(const std::map<std::string, std::string>& headers) {
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) {
        std::string headerName = toLowerCase(it->first);
        if (headerName == "content-type") {
            return it->second;
        }
    }
    return "";
}


Response* POSThandler::createErrorResponse(int statusCode, const std::string& message) {
    Response* response = new Response();
    response->setStatus(statusCode);
    response->setVersion("HTTP/1.0");
    response->setServer("WebServer/1.0");
    response->setDate();
    response->setConnection("close");
    
    std::string body = "<html><body><h1>" + numberToString(statusCode) + " " + message + "</h1></body></html>";
    response->setBody(body);
    
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "text/html";
    headers["Content-Length"] = numberToString(body.length());
    response->setHeaders(headers);
    
    return response;
}