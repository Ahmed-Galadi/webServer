#include "./POSThandler.hpp"
#include "../../../../include/GlobalUtils.hpp"

Response* POSThandler::handleFormUrlEncoded(const Request& req, Response* response,
                                         const std::vector<RequestBody>& bodyParts,
                                         const std::string& uri) {
    
    std::string responseBody = "<html><body><h1>POST Success - Form Data</h1>";
    responseBody += "<h2>URI: " + uri + "</h2>";
    responseBody += "<h2>Received Form Data:</h2>";
    responseBody += "<table border='1' cellpadding='5' cellspacing='0'>";
    responseBody += "<tr><th>Field Name</th><th>Value</th></tr>";
    
    size_t totalFields = 0;
    
    
    for (size_t i = 0; i < bodyParts.size(); ++i) {
        const RequestBody& part = bodyParts[i];
        
        std::string fieldName = part.getName();
        std::string fieldValue = part.getRawData();
        
        
        responseBody += "<tr>";
        responseBody += "<td><strong>" + fieldName + "</strong></td>";
        
        if (fieldValue.length() > 200) {
            fieldValue = fieldValue.substr(0, 200) + "... (truncated)";
        }
        responseBody += "<td>" + fieldValue + "</td>";
        responseBody += "</tr>";
        totalFields++;
        
        std::map<std::string, std::string> encodedData = part.getEncodedData();
        if (!encodedData.empty()) {
            std::map<std::string, std::string>::const_iterator it;
            for (it = encodedData.begin(); it != encodedData.end(); ++it) {
                
                responseBody += "<tr>";
                responseBody += "<td><strong>" + it->first + " (encoded)</strong></td>";
                
                std::string value = it->second;
                if (value.length() > 200) {
                    value = value.substr(0, 200) + "... (truncated)";
                }
                responseBody += "<td>" + value + "</td>";
                responseBody += "</tr>";
                totalFields++;
            }
        }
    }
    
    responseBody += "</table>";
    responseBody += "<p><strong>Total Fields:</strong> " + numberToString(totalFields) + "</p>";
    
    responseBody += "<div style='background-color: #f0f0f0; padding: 10px; margin: 10px 0;'>";
    responseBody += "<h3>Debug Information:</h3>";
    responseBody += "<p><strong>Body Parts Count:</strong> " + numberToString(bodyParts.size()) + "</p>";
    
    std::string rawBody = req.getRawBody();
    responseBody += "<p><strong>Raw Body Length:</strong> " + numberToString(rawBody.length()) + " bytes</p>";
    
    if (rawBody.length() > 0 && rawBody.length() <= 500) {
        responseBody += "<p><strong>Raw Body Content:</strong></p>";
        responseBody += "<pre style='background-color: #eee; padding: 5px;'>" + rawBody + "</pre>";
    } else if (rawBody.length() > 500) {
        responseBody += "<p><strong>Raw Body (first 500 chars):</strong></p>";
        responseBody += "<pre style='background-color: #eee; padding: 5px;'>" + rawBody.substr(0, 500) + "...</pre>";
    }
    responseBody += "</div>";
    
    responseBody += "<div style='margin: 20px 0;'>";
    responseBody += "<a href='/'>← Back to Home</a>";
    responseBody += "</div>";
    
    responseBody += "</body></html>";
    
    response->setStatus(200);
    response->setBody(responseBody);
    
    std::map<std::string, std::string> responseHeaders;
    responseHeaders["Content-Type"] = "text/html";
    responseHeaders["Content-Length"] = numberToString(responseBody.length());
    response->setHeaders(responseHeaders);
    
    return response;
}