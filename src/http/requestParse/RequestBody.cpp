#include "RequestBody.hpp"
#include <iostream>
#include "../../config/ParseUtils.hpp"
#include "RequestParser.hpp"

void RequestBody::setName(const std::string &name) {
    this->name = name;
}

void RequestBody::setFileName(const std::string &fileName) {
    this->fileName = fileName;
}

void RequestBody::setContentType(const std::string &contentType) {
    this->contentType = contentType;
}

void RequestBody::setBinaryData(const std::vector<char>& data) {
    binaryData = data;
    
    rawData = std::string(data.begin(), data.end());
}

void RequestBody::setBinaryData(const char* data, size_t size) {
    binaryData.clear();
    binaryData.reserve(size);
    
    for (size_t i = 0; i < size; ++i) {
        binaryData.push_back(data[i]);
    }
    
    rawData.assign(data, size);
}

void RequestBody::setRawData(const std::string &data) {
    rawData = data;
    
    binaryData.clear();
    binaryData.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        binaryData.push_back(data[i]);
    }
}

void RequestBody::setEncodedData(const std::map<std::string, std::string> &encodedData) {
    this->encodData = encodedData;
}

std::string RequestBody::getName() const {
    return name;
}

std::string RequestBody::getFileName() const {
    return fileName;
}

std::string RequestBody::getContentType() const {
    return contentType;
}

std::string RequestBody::getRawData() const {
    return rawData;
}

const std::vector<char>& RequestBody::getBinaryData() const {
    return binaryData;
}

std::string RequestBody::getBinaryDataAsString() const {
    if (!binaryData.empty()) {
        return std::string(binaryData.begin(), binaryData.end());
    }
    return rawData;
}

std::map<std::string, std::string> RequestBody::getEncodedData() const {
    return encodData;
}

bool RequestBody::isBinaryData() const {
    if (binaryData.empty()) return false;
    
    for (size_t i = 0; i < binaryData.size(); ++i) {
        if (binaryData[i] == '\0') {
            return true;
        }
    }
    return false;
}

size_t RequestBody::getDataSize() const {
    return binaryData.empty() ? rawData.size() : binaryData.size();
}