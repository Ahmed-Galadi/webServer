#include "./POSThandler.hpp"
#include "../../../../include/GlobalUtils.hpp"

// Handler for text/plain content
Response* POSThandler::handlePlainText(const Request& req, Response* response, const std::string& uri) {
    std::string textData = req.getRawBody();
    
    std::string responseBody = "POST Success - Plain Text\n";
    responseBody += "URI: " + uri + "\n";
    responseBody += "Received: " + numberToString(textData.length()) + " bytes of text data\n\n";
    
    // Show first 500 characters
    if (textData.length() > 0) {
        responseBody += "Content Preview:\n";
        responseBody += "================\n";
        std::string preview = textData.substr(0, 500);
        responseBody += preview;
        if (textData.length() > 500) {
            responseBody += "\n... (truncated)";
        }
    } else {
        responseBody += "No content received.";
    }
    
    // TODO: Here you could process the text data
    // Example: processTextData(textData, uri);
    
    response->setStatus(200);
    response->setBody(responseBody);
    
    std::map<std::string, std::string> responseHeaders;
    responseHeaders["Content-Type"] = "text/plain";
    responseHeaders["Content-Length"] = numberToString(responseBody.length());
    response->setHeaders(responseHeaders);
    
    return response;
}

// Handler for binary data (application/octet-stream)
Response* POSThandler::handleBinaryData(const Request& req, Response* response,
                                     const std::vector<RequestBody>& bodyParts,
                                     const std::string& uri) {
    
    std::string responseBody = "<html><body><h1>POST Success - Binary Data</h1>";
    responseBody += "<h2>URI: " + uri + "</h2>";
    
    size_t totalBytes = 0;
    
    if (!bodyParts.empty()) {
        responseBody += "<h2>Binary Parts Received:</h2>";
        responseBody += "<ul>";
        
        for (size_t i = 0; i < bodyParts.size(); ++i) {
            const RequestBody& part = bodyParts[i];
            size_t partSize = part.getRawData().length();
            totalBytes += partSize;
            
            responseBody += "<li>Part " + numberToString(i + 1) + ": ";
            responseBody += numberToString(partSize) + " bytes";
            
            if (!part.getName().empty()) {
                responseBody += " (Name: " + part.getName() + ")";
            }
            if (!part.getContentType().empty()) {
                responseBody += " (Type: " + part.getContentType() + ")";
            }
            responseBody += "</li>";
        }
        responseBody += "</ul>";
    } else {
        // Handle raw binary data
        std::string rawData = req.getRawBody();
        totalBytes = rawData.length();
        responseBody += "<p>Raw binary data: " + numberToString(totalBytes) + " bytes</p>";
    }
    
    responseBody += "<p><strong>Total Size:</strong> " + numberToString(totalBytes) + " bytes</p>";
    
    // TODO: Here you could save or process the binary data
    // Example: saveBinaryData(bodyParts, uri);
    
    responseBody += "</body></html>";
    
    response->setStatus(200);
    response->setBody(responseBody);
    
    std::map<std::string, std::string> responseHeaders;
    responseHeaders["Content-Type"] = "text/html";
    responseHeaders["Content-Length"] = numberToString(responseBody.length());
    response->setHeaders(responseHeaders);
    
    return response;
}

// Default handler for unknown content types
Response* POSThandler::handleDefaultPost(const Request& req, Response* response, const std::string& uri) {
    std::string rawBody = req.getRawBody();
    std::map<std::string, std::string> headers = req.getHeaders();
    
    std::string responseBody = "<html><body><h1>POST Received</h1>";
    responseBody += "<h2>Request Details:</h2>";
    responseBody += "<p><strong>URI:</strong> " + uri + "</p>";
    responseBody += "<p><strong>Method:</strong> " + req.getMethod() + "</p>";
    responseBody += "<p><strong>Version:</strong> " + req.getVersion() + "</p>";
    responseBody += "<p><strong>Content Length:</strong> " + numberToString(rawBody.length()) + " bytes</p>";
    
    // Show headers
    responseBody += "<h3>Headers:</h3><ul>";
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) {
        responseBody += "<li><strong>" + it->first + ":</strong> " + it->second + "</li>";
    }
    responseBody += "</ul>";
    
    // Show query parameters if any
    std::map<std::string, std::string> query = req.getQuery();
    if (!query.empty()) {
        responseBody += "<h3>Query Parameters:</h3><ul>";
        for (it = query.begin(); it != query.end(); ++it) {
            responseBody += "<li><strong>" + it->first + ":</strong> " + it->second + "</li>";
        }
        responseBody += "</ul>";
    }
    
    // Show body preview
    if (rawBody.length() > 0) {
        responseBody += "<h3>Body Preview:</h3>";
        responseBody += "<pre>";
        std::string preview = rawBody.substr(0, 300);
        responseBody += preview;
        if (rawBody.length() > 300) {
            responseBody += "\n... (truncated)";
        }
        responseBody += "</pre>";
    }
    
    responseBody += "</body></html>";
    
    response->setStatus(200);
    response->setBody(responseBody);
    
    std::map<std::string, std::string> responseHeaders;
    responseHeaders["Content-Type"] = "text/html";
    responseHeaders["Content-Length"] = numberToString(responseBody.length());
    response->setHeaders(responseHeaders);
    
    return response;
}