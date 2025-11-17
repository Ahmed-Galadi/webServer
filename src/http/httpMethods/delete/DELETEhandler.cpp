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
    std::string filePath = FileHandler::resolveFilePath(uri, location);

    
    if (!fileExists(filePath)) {
        return Response::makeErrorResponse(404);
    }
    
    if (std::remove(filePath.c_str()) != 0) {
        return Response::makeErrorResponse(500);
    }
    
    
    Response* response = new Response();
    response->setStatus(204);
    response->setVersion("HTTP/1.0");
    response->setServer("WebServer/1.0");
    response->setDate();
    response->setConnection("close");
    response->setBody("");
    
    std::map<std::string, std::string> headers;
    headers["Content-Length"] = "0";
    response->setHeaders(headers);
    
    return response;
}
