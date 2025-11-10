#include "./POSThandler.hpp"

Response* POSThandler::handleJsonData(const Request& req, Response* response, const std::string& uri) {
    std::string jsonData = req.getRawBody();
    
    // Basic JSON validation (check if it starts and ends with braces or brackets)
    bool isValidJson = false;
    if (!jsonData.empty()) {
        char first = jsonData[0];
        char last = jsonData[jsonData.length() - 1];
        isValidJson = (first == '{' && last == '}') || (first == '[' && last == ']');
    }
    
    // Create JSON response
    std::string responseBody = "{";
    responseBody += "\"status\":\"success\",";
    responseBody += "\"message\":\"JSON data received\",";
    responseBody += "\"uri\":\"" + uri + "\",";
    responseBody += "\"data_length\":" + numberToString(jsonData.length()) + ",";
    responseBody += "\"valid_json\":" + std::string(isValidJson ? "true" : "false");
    
    // Add preview of received data (first 100 characters)
    if (jsonData.length() > 0) {
        std::string preview = jsonData.substr(0, 100);
        // Escape quotes in preview for JSON
        std::string escapedPreview = "";
        for (size_t i = 0; i < preview.length(); ++i) {
            if (preview[i] == '"') {
                escapedPreview += "\\\"";
            } else if (preview[i] == '\\') {
                escapedPreview += "\\\\";
            } else if (preview[i] == '\n') {
                escapedPreview += "\\n";
            } else if (preview[i] == '\r') {
                escapedPreview += "\\r";
            } else if (preview[i] == '\t') {
                escapedPreview += "\\t";
            } else {
                escapedPreview += preview[i];
            }
        }
        responseBody += ",\"preview\":\"" + escapedPreview;
        if (jsonData.length() > 100) {
            responseBody += "...";
        }
        responseBody += "\"";
    }
    
    responseBody += "}";
    
    // TODO: Here you could parse the JSON and process it
    // Example: processJsonData(jsonData, uri);
    
    response->setStatus(200);
    response->setBody(responseBody);
    
    std::map<std::string, std::string> responseHeaders;
    responseHeaders["Content-Type"] = "application/json";
    responseHeaders["Content-Length"] = numberToString(responseBody.length());
    response->setHeaders(responseHeaders);
    
    return response;
}