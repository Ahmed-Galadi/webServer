
#include "./POSThandler.hpp"
#include "../../../../include/GlobalUtils.hpp"
#include "../utils/FileHandler.hpp"
#include "../utils/MimeType.hpp"
#include <sstream>



Response* POSThandler::handleMultipartFormData(const Request& /* req */, Response* response, 
                                            const std::vector<RequestBody>& bodyParts, 
                                            const std::string& uri,
                                            const LocationConfig* location) {
    
    
    std::string uploadDir = getUploadDirectory(location);;
    FileHandler::setUploadDirectory(uploadDir);
    
    std::string responseBody = "<html><body><h1>POST Success - Multipart Data</h1>";
    responseBody += "<h2>URI: " + uri + "</h2>";
    responseBody += "<h2>Upload Directory: " + uploadDir + "</h2>";
    responseBody += "<h2>Processing Results:</h2>";
    
    std::vector<std::string> uploadedFiles;
    size_t totalFiles = 0;
    size_t totalFields = 0;
    for (size_t i = 0; i < bodyParts.size(); ++i) {
        const RequestBody& part = bodyParts[i];
        responseBody += "<div style='border: 1px solid #ccc; margin: 10px; padding: 10px;'>";
        responseBody += "<p><strong>Field Name:</strong> " + part.getName() + "</p>";
        
        if (!part.getFileName().empty()) {
            totalFiles++;
            std::string originalFilename = part.getFileName();
            std::string contentType = part.getContentType();
            
            size_t fileSize;
            bool hasBinaryData = false;
            
            try {
                const std::vector<char>& binaryData = part.getBinaryData();
                fileSize = binaryData.size();
                hasBinaryData = !binaryData.empty();
            } catch (...) {
                fileSize = part.getRawData().size();
                hasBinaryData = false;
            }
            
            responseBody += "<p><strong>Original Filename:</strong> " + originalFilename + "</p>";
            responseBody += "<p><strong>Content-Type:</strong> " + contentType + "</p>";
            responseBody += "<p><strong>File Size:</strong> " + numberToString(fileSize) + " bytes</p>";
            
            std::string savedFilename;
            if (hasBinaryData) {
                const std::vector<char>& binaryData = part.getBinaryData();
                savedFilename = FileHandler::saveUploadedBinaryFile(originalFilename, binaryData);
            } else {
                std::string textData = part.getRawData();
                savedFilename = FileHandler::saveUploadedFile(originalFilename, textData);
            }
            
            if (!savedFilename.empty()) {
                uploadedFiles.push_back(savedFilename);
                responseBody += "<p><strong>Status:</strong> <span style='color: green;'>✓ Uploaded Successfully</span></p>";
                responseBody += "<p><strong>Saved As:</strong> " + savedFilename + "</p>";
            } else {
                responseBody += "<p><strong>Status:</strong> <span style='color: red;'>✗ Upload Failed</span></p>";
            }
        } else {
            totalFields++;
            std::string value = part.getRawData();
            if (value.length() > 200) {
                value = value.substr(0, 200) + "... (truncated)";
            }
            responseBody += "<p><strong>Value:</strong> " + value + "</p>";
        }
        responseBody += "</div>";
    }
    
    responseBody += "</body></html>";
    
    if (!uploadedFiles.empty()) {
        response->setStatus(201);
        
        std::map<std::string, std::string> responseHeaders;
        responseHeaders["Content-Type"] = "text/html";
        responseHeaders["Content-Length"] = numberToString(responseBody.length());
        
        responseHeaders["Location"] = FileHandler::getUploadDirectory() + "/" + uploadedFiles[0];
        
        response->setHeaders(responseHeaders);
    } else {
        response->setStatus(200);
        
        std::map<std::string, std::string> responseHeaders;
        responseHeaders["Content-Type"] = "text/html";
        responseHeaders["Content-Length"] = numberToString(responseBody.length());
        response->setHeaders(responseHeaders);
    }

    response->setBody(responseBody);
    return response;
}

