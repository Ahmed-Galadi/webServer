#include "Request.hpp"
#include <iostream>
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include "../../config/ParseUtils.hpp"
#include "RequestParser.hpp"
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>

std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
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

void Request::extractRequestData(std::string rawReqData){
    std::vector<std::string> parts = split(rawReqData, " ");
    
    if (parts.size() < 3) {
        throw InvalidRequest();
    }

    method = parts[0];
    uri = parts[1];
    version = parts[2];
}

Request::Request(std::string rawRequest) {
    if (!isCompleteRequest(rawRequest)) {
        throw IncompleteRequest();
    }
    
    parseRawReq(rawRequest);
}

bool Request::isCompleteRequest(const std::string& rawRequest) const {
    size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false;
    }
    
    std::string headersSection = rawRequest.substr(0, headerEnd);
    size_t contentLength = getContentLengthFromHeaders(headersSection);
    size_t totalHeadersSize = headerEnd + 4;
    size_t expectedTotalSize = totalHeadersSize + contentLength;
    
    bool isComplete = rawRequest.size() >= expectedTotalSize;
    
    return isComplete;
}

size_t Request::findHeaderEnd(const std::string& rawRequest) const {
    return rawRequest.find("\r\n\r\n");
}

size_t Request::getContentLengthFromHeaders(const std::string& headersSection) const {
    std::string lowerHeaders = headersSection;
    std::transform(lowerHeaders.begin(), lowerHeaders.end(), lowerHeaders.begin(), ::tolower);
    
    size_t pos = lowerHeaders.find("content-length:");
    if (pos == std::string::npos) {
        return 0;
    }
    
    size_t valueStart = pos + 15;
    size_t valueEnd = headersSection.find("\r\n", valueStart);
    
    if (valueEnd == std::string::npos) {
        return 0;
    }
    
    std::string lengthStr = headersSection.substr(valueStart, valueEnd - valueStart);
    
    size_t first = lengthStr.find_first_not_of(" \t");
    size_t last = lengthStr.find_last_not_of(" \t");
    if (first != std::string::npos && last != std::string::npos) {
        lengthStr = lengthStr.substr(first, last - first + 1);
    } else if (first != std::string::npos) {
        lengthStr = lengthStr.substr(first);
    }
    
    size_t result = static_cast<size_t>(std::atol(lengthStr.c_str()));
    
    return result;
}

size_t Request::getContentLength() const {
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) {
        std::string headerName = it->first;
        std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
        if (headerName == "content-length") {
            return static_cast<size_t>(atol(it->second.c_str()));
        }
    }
    return 0;
}

void Request::parseRawReq(std::string rawRequest) {
    size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        throw InvalidRequest();
    }
    
    std::string headersData = rawRequest.substr(0, headerEnd);
    
    std::vector<std::string> lines = split(headersData, "\r\n");
    if (lines.empty()) {
        throw InvalidRequest();
    }
    
    Request::extractRequestData(lines[0]);
    
    std::vector<std::string> headerLines(lines.begin() + 1, lines.end());
    extractHeaders(headerLines);
    
    size_t bodyStart = headerEnd + 4;
    if (bodyStart < rawRequest.size()) {
        size_t bodySize = rawRequest.size() - bodyStart;
        
        binaryBody.clear();
        binaryBody.reserve(bodySize);
        for (size_t i = bodyStart; i < rawRequest.size(); ++i) {
            binaryBody.push_back(rawRequest[i]);
        }
        
        body = rawRequest.substr(bodyStart);
    }
}

void Request::extractBinaryBody(const char* data, size_t size) {
    binaryBody.clear();
    binaryBody.reserve(size);
    
    for (size_t i = 0; i < size; ++i) {
        binaryBody.push_back(data[i]);
    }
    
    body.assign(data, size);
}

std::vector<std::string> Request::splitHeaderFromBody(std::string rawRequest) {
    std::vector<std::string> parts;
    size_t pos = rawRequest.find("\r\n\r\n");
    
    if (pos == std::string::npos) {
        parts.push_back(rawRequest);
        return parts;
    }
    
    std::string headers = rawRequest.substr(0, pos);
    
    std::string body = rawRequest.substr(pos + 4);
    
    parts.push_back(headers);
    if (!body.empty()) {
        parts.push_back(body);
    }
    
    return parts;
}

void Request::extractBody(std::string rawBody) {
    binaryBody.clear();
    binaryBody.reserve(rawBody.size());
    
    for (size_t i = 0; i < rawBody.size(); ++i) {
        binaryBody.push_back(rawBody[i]);
    }
    
    body = rawBody;
}

void Request::extractQuery(std::string queryString) {
    std::vector<std::string> splitedQuery = ParseUtils::splitString(queryString, '&');
    std::map<std::string, std::string> outputQuery;
    std::pair<std::string, std::string> holderQuery;

    for (size_t i = 0; i < splitedQuery.size(); i++) {
        std::vector<std::string> tmp = ParseUtils::splitString(splitedQuery[i], '=');
        if (tmp.size() != 2)
            throw (InvalidRequest());
        holderQuery = std::make_pair(tmp[0], tmp[1]);
        outputQuery.insert(holderQuery);
    }

    this->query = outputQuery;
}

void Request::extractHeaders(std::vector<std::string> splitedHeaders) {
    for (size_t i = 0; i < splitedHeaders.size(); ++i) {
        std::string line = ParseUtils::trim(splitedHeaders[i]);
        if (line.empty())
            continue;

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            throw (InvalidRequest());

        std::string key = ParseUtils::trim(line.substr(0, colon));
        std::string value = ParseUtils::trim(line.substr(colon + 1));
        this->headers.insert(std::make_pair(key, value));
    }
}

std::map<std::string, std::string> Request::getHeaders() const {
    return headers;
}

std::string Request::getMethod() const {
    return method;
}

std::string Request::getURI() const {
    return uri;
}

std::string Request::getVersion() const {
    return version;
}

std::vector<RequestBody> Request::getBody() {
    if (method == "POST")
        return RequestParser::ParseBody(*this);
    throw (ForbiddenMethod());
}

std::map<std::string, std::string> Request::getQuery() const {
    return query;
}

std::string Request::getRawBody() const {
    if (!binaryBody.empty()) {
        return std::string(binaryBody.begin(), binaryBody.end());
    }
    return body;
}

const std::vector<char>& Request::getRawBinaryBody() const {
    return binaryBody;
}

const char* Request::InvalidRequest::what() const throw() {
    return ("\033[31m[REQUEST ERROR: INVALID REQUEST!]\033[0m");
}

const char* Request::IncompleteRequest::what() const throw() {
    return ("\033[33m[REQUEST WARNING: INCOMPLETE REQUEST!]\033[0m");
}

const char* Request::ForbiddenMethod::what() const throw() {
    return ("\033[31m[REQUEST ERROR: FORBIDDEN METHOD?]\033[0m");
}

const char* Request::NotSupportedRequest::what() const throw() {
    return ("\033[31m[REQUEST ERROR: NOT SUPPORTED!]\033[0m");
}
