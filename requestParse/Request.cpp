
// Request.cpp
#include "Request.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include "../configParser/ParseUtils.hpp"

static std::string trim(const std::string &s) {
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start])) ++start;
    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1])) --end;
    return s.substr(start, end - start);
}

Request::Request(std::string rawRequest) {
    parseRawReq(rawRequest);
}

void Request::parseRawReq(std::string rawRequest) {
    std::vector<std::string> separateData = splitHeaderFromBody(rawRequest);
    std::vector<std::string> headers;

    if (separateData.size() < 2) {
        std::cout << "Error: incomplete header" << std::endl;
        exit(1);
    } else {
        extractRequestData(separateData[0]);
        headers = ParseUtils::splitString(separateData[1], '\n');
        extractHeaders(headers);
        if (separateData.size() == 3)
            extractBody(separateData[2]);
        if (separateData.size() > 3) {
            std::cout << "Error: Weird Request!" << std::endl;
            exit(1);
        }
    }
}

std::vector<std::string> Request::splitHeaderFromBody(std::string rawRequest) {
    std::vector<std::string> output;
    size_t delPos = rawRequest.find("\r\n\r\n");
    if (delPos == std::string::npos) {
        std::cout << "Error: Incomplete Request" << std::endl;
        exit(1);
    }

    std::string headerBlock = rawRequest.substr(0, delPos);


    size_t firstCRLF = headerBlock.find("\r\n");
    if (firstCRLF == std::string::npos) {
        std::cout << "Error: no request line" << std::endl;
        exit(1);
    }

    std::string requestLine = headerBlock.substr(0, firstCRLF);
    std::string headerLines = headerBlock.substr(firstCRLF + 2);

    output.push_back(requestLine);
    output.push_back(headerLines);

    std::string bodyHolder = rawRequest.substr(delPos + 4);
    if (!bodyHolder.empty())
        output.push_back(bodyHolder);

    return output;
}

void Request::extractRequestData(std::string rawReqData) {
    std::vector<std::string> splitedReqData = ParseUtils::splitString(rawReqData, ' ');

    if (splitedReqData.size() < 3) {
        std::cout << "Error: request data is not properly set!" << std::endl;
        exit(1);
    }
    this->method = trim(splitedReqData[0]);
    this->uri = trim(splitedReqData[1]);
    this->version = trim(splitedReqData[2]);
}

void Request::extractHeaders(std::vector<std::string> splitedHeaders) {
    for (size_t i = 0; i < splitedHeaders.size(); ++i) {
        std::string line = trim(splitedHeaders[i]);
        if (line.empty())
            continue;
        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            std::cout << "Error: Incomplete header" << std::endl;
            exit(1);
        }
        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        this->headers.insert(std::make_pair(key, value));
    }
}

void Request::extractBody(std::string rawBody) {
    this->body = rawBody;
}

std::map<std::string, std::string> Request::getHeaders() const {
	return (headers);
}

std::string	Request::getMethod() const {
	return (method);
}

std::string	Request::getURI() const {
	return (uri);
}

std::string	Request::getVersion() const {
	return (version);
}

std::string	Request::getBody() const {
	return (body);
}