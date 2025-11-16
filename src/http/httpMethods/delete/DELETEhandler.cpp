// DELETEhandler.cpp - Fixed version

#include "./DELETEhandler.hpp"
#include <sys/stat.h>
#include <iostream>
#include <cstdio>

static bool fileExists(const std::string &path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

Response* DELETEhandler::handler(const Request &req, const LocationConfig* location, const ServerConfig* /* serverConfig */) {
    
    std::string uri = req.getURI();
    
    // ============================================
    // STEP 1: Build file path using same logic as GET
    // ============================================
    
    // Get root from location (must match dispatcher's logic)
    std::string root = location->getRoot();
    if (root.empty()) {
        root = "www";  // Fallback
    }
    
    
    // Remove location path prefix from URI
    std::string locationPath = location->getPath();
    std::string relativePath = uri;
    
    if (uri.find(locationPath) == 0) {
        relativePath = uri.substr(locationPath.length());
    }
    
    
    // Build full file path

    std::string filePath = FileHandler::resolveFilePath(uri, location);

    
    
    // ============================================
    // STEP 2: Validate file exists (should already be checked by dispatcher)
    // ============================================
    
    if (!fileExists(filePath)) {
        return Response::makeErrorResponse(404);
    }
    
    // ============================================
    // STEP 3: Delete the file
    // ============================================
    
    if (std::remove(filePath.c_str()) != 0) {
        return Response::makeErrorResponse(500);
    }
    
    
    // ============================================
    // STEP 4: Return 204 No Content
    // ============================================
    
    Response* response = new Response();
    response->setStatus(204);
    response->setVersion("HTTP/1.0");
    response->setServer("WebServer/1.0");
    response->setDate();
    response->setConnection("close");
    response->setBody("");  // No body for 204
    
    std::map<std::string, std::string> headers;
    headers["Content-Length"] = "0";
    response->setHeaders(headers);
    
    return response;
}
