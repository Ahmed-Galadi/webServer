#pragma once

#include "../../../../include/webserv.hpp"
#include "../../response/HttpMethodHandler.hpp"
#include "../../requestParse/Request.hpp"
#include "../../requestParse/RequestBody.hpp"
#include "../../response/Response.hpp"
#include "../../../config/LocationConfig.hpp"
#include "../utils/FileHandler.hpp"

class POSThandler : public HttpMethodHandler {
private:
    Response* handleMultipartFormData(const Request& req, Response* response, 
                                    const std::vector<RequestBody>& bodyParts, 
                                    const std::string& uri,
                                    const LocationConfig* location);
    
    Response* handleFormUrlEncoded(const Request& req, Response* response,
                                 const std::vector<RequestBody>& bodyParts,
                                 const std::string& uri);
    
    Response* handleJsonData(const Request& req, Response* response, const std::string& uri);
    
    Response* handlePlainText(const Request& req, Response* response, const std::string& uri);
    
    Response* handleBinaryData(const Request& req, Response* response,
                             const std::vector<RequestBody>& bodyParts,
                             const std::string& uri);
    
    Response* handleDefaultPost(const Request& req, Response* response, const std::string& uri);
    
    std::string getContentType(const std::map<std::string, std::string>& headers);
    
    Response* createErrorResponse(int statusCode, const std::string& message);
    std::string getUploadDirectory(const LocationConfig* location) const;

public:
    Response* handler(const Request& req, const LocationConfig* location, const ServerConfig* serverConfig);
};