#include "RequestParser.hpp"
#include "../../config/ParseUtils.hpp"
#include <iostream>
#include <sstream>

std::vector<RequestBody> RequestParser::parseMultipartFormData(const Request& req, const std::string& contentType) {
    
    // Extract boundary
    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return std::vector<RequestBody>();
    }
    
    
    // BINARY-SAFE: Use binary data for parsing
    const std::vector<char>& binaryBody = req.getRawBinaryBody();
    if (binaryBody.empty()) {
        return std::vector<RequestBody>();
    }
    
    
    return parseMultipartBinary(binaryBody, boundary);
}

std::vector<RequestBody> RequestParser::ParseBody(const Request& req) {
    std::vector<RequestBody> bodies;
    
    std::map<std::string, std::string> headers = req.getHeaders();
    std::map<std::string, std::string>::iterator it = headers.find("Content-Type");
    
    if (it == headers.end()) {
        return bodies;
    }
    
    std::string contentType = it->second;
    
    if (contentType.find("multipart/form-data") != std::string::npos) {
        return RequestParser::parseMultipartFormData(req, contentType);
    } else if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
        return RequestParser::parseUrlEncodedData(req);
    }
    
    return bodies;
}



std::vector<RequestBody> RequestParser::parseMultipartBinary(const std::vector<char>& data, const std::string& boundary)
{
    std::vector<RequestBody> bodies;
    
    std::string fullBoundary = "--" + boundary;
    std::string endBoundary = "--" + boundary + "--";
    
    
    // Convert binary data to string for boundary searching (this is safe because boundaries are ASCII)
    std::string dataStr(data.begin(), data.end());
    
    std::vector<size_t> boundaryPositions;
    
    // Find all boundary positions
    size_t pos = 0;
    while ((pos = dataStr.find(fullBoundary, pos)) != std::string::npos) {
        boundaryPositions.push_back(pos);
        pos += fullBoundary.length();
    }
    
    if (boundaryPositions.size() < 2) {
        return bodies;
    }
    
    // Parse each part between boundaries
    for (size_t i = 0; i < boundaryPositions.size() - 1; ++i) {
        size_t partStart = boundaryPositions[i] + fullBoundary.length();
        size_t partEnd = boundaryPositions[i + 1];
        
        // Skip CRLF after boundary
        if (partStart + 2 < data.size() && data[partStart] == '\r' && data[partStart + 1] == '\n') {
            partStart += 2;
        }
        
        if (partStart >= partEnd) continue;
        
        
        RequestBody part = parseMultipartPart(data, partStart, partEnd);
        if (!part.getName().empty()) {
            bodies.push_back(part);
        }
    }
    
    return bodies;
}

RequestBody RequestParser::parseMultipartPart(const std::vector<char>& data, size_t start, size_t end) {
    RequestBody body;
    
    if (start >= end || end > data.size()) {
        return body;
    }
    
    // Find headers end (\r\n\r\n) within this part
    std::string partStr(data.begin() + start, data.begin() + end);
    size_t headersEnd = partStr.find("\r\n\r\n");
    
    if (headersEnd == std::string::npos) {
        return body;
    }
    
    std::string headersStr = partStr.substr(0, headersEnd);
    
    // Parse headers
    parsePartHeaders(headersStr, body);
    
    // Extract binary data
    size_t dataStart = start + headersEnd + 4; // +4 for \r\n\r\n
    size_t dataEnd = end;
    
    // Remove trailing CRLF before next boundary
    if (dataEnd >= 2 && data[dataEnd - 2] == '\r' && data[dataEnd - 1] == '\n') {
        dataEnd -= 2;
    }
    
    if (dataStart < dataEnd) {
        size_t dataSize = dataEnd - dataStart;
        
        // BINARY-SAFE: Store the data properly
        body.setBinaryData(&data[dataStart], dataSize);
        
        
        if (!body.getFileName().empty()) {
        }
    }
    
    return body;
}

void RequestParser::parsePartHeaders(const std::string& headersStr, RequestBody& body) {
    std::vector<std::string> headerLines = split(headersStr, "\r\n");
    
    for (size_t i = 0; i < headerLines.size(); ++i) {
        std::string line = ParseUtils::trim(headerLines[i]);
        if (line.empty()) continue;
        
        
        if (line.find("Content-Disposition:") == 0) {
            parseContentDisposition(line, body);
        } else if (line.find("Content-Type:") == 0) {
            parseContentType(line, body);
        }
    }
}

void RequestParser::parseContentDisposition(const std::string& header, RequestBody& body) {
    // Example: Content-Disposition: form-data; name="file"; filename="test.jpg"
    
    size_t namePos = header.find("name=\"");
    if (namePos != std::string::npos) {
        namePos += 6; // Skip name="
        size_t nameEnd = header.find("\"", namePos);
        if (nameEnd != std::string::npos) {
            std::string name = header.substr(namePos, nameEnd - namePos);
            body.setName(name);
        }
    }
    
    size_t filenamePos = header.find("filename=\"");
    if (filenamePos != std::string::npos) {
        filenamePos += 10; // Skip filename="
        size_t filenameEnd = header.find("\"", filenamePos);
        if (filenameEnd != std::string::npos) {
            std::string filename = header.substr(filenamePos, filenameEnd - filenamePos);
            body.setFileName(filename);
        }
    }
}

void RequestParser::parseContentType(const std::string& header, RequestBody& body) {
    // Example: Content-Type: image/jpeg
    
    size_t colonPos = header.find(":");
    if (colonPos != std::string::npos && colonPos + 1 < header.length()) {
        std::string contentType = ParseUtils::trim(header.substr(colonPos + 1));
        body.setContentType(contentType);
    }
}

std::string RequestParser::extractBoundary(const std::string& contentType) {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "";
    }
    
    boundaryPos += 9; // Skip "boundary="
    
    // Handle quoted boundary
    std::string boundary;
    if (boundaryPos < contentType.length()) {
        if (contentType[boundaryPos] == '"') {
            // Quoted boundary
            boundaryPos++; // Skip opening quote
            size_t endPos = contentType.find('"', boundaryPos);
            if (endPos != std::string::npos) {
                boundary = contentType.substr(boundaryPos, endPos - boundaryPos);
            }
        } else {
            // Unquoted boundary - take until semicolon or end
            size_t endPos = contentType.find(';', boundaryPos);
            if (endPos == std::string::npos) {
                boundary = contentType.substr(boundaryPos);
            } else {
                boundary = contentType.substr(boundaryPos, endPos - boundaryPos);
            }
            boundary = ParseUtils::trim(boundary);
        }
    }
    
    return boundary;
}

std::vector<RequestBody> RequestParser::parseUrlEncodedData(const Request& req) {
    std::vector<RequestBody> bodies;
    
    std::string bodyData = req.getRawBody();
    std::vector<std::string> pairs = split(bodyData, "&");
    
    for (size_t i = 0; i < pairs.size(); ++i) {
        std::vector<std::string> keyValue = split(pairs[i], "=");
        if (keyValue.size() == 2) {
            RequestBody body;
            body.setName(urlDecode(keyValue[0]));
            body.setRawData(urlDecode(keyValue[1]));
            bodies.push_back(body);
        }
    }
    
    return bodies;
}

std::string RequestParser::urlDecode(const std::string& encoded) {
    std::string decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            // Convert hex to char
            std::string hex = encoded.substr(i + 1, 2);
            char c = static_cast<char>(std::strtol(hex.c_str(), NULL, 16));
            decoded += c;
            i += 2;
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    return decoded;
}

// Helper function for splitting strings
std::vector<std::string> RequestParser::split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    
    tokens.push_back(str.substr(start));
    return tokens;
}