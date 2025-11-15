#include "RequestBody.hpp"
#include <iostream>
#include "../../config/ParseUtils.hpp"
#include "RequestParser.hpp"

// Setters
void RequestBody::setName(const std::string &name) {
    this->name = name;
}

void RequestBody::setFileName(const std::string &fileName) {
    this->fileName = fileName;
}

void RequestBody::setContentType(const std::string &contentType) {
    this->contentType = contentType;
}

// BINARY-SAFE: New method for setting raw binary data
void RequestBody::setBinaryData(const std::vector<char>& data) {
    binaryData = data;
    
    // Also create string version for backwards compatibility
    rawData = std::string(data.begin(), data.end());
}

// BINARY-SAFE: Enhanced version with explicit size
void RequestBody::setBinaryData(const char* data, size_t size) {
    binaryData.clear();
    binaryData.reserve(size);
    
    for (size_t i = 0; i < size; ++i) {
        binaryData.push_back(data[i]);
    }
    
    // Create string version - use assign with explicit size to handle null bytes
    rawData.assign(data, size);
}

void RequestBody::setRawData(const std::string &data) {
    rawData = data;
    
    // Also store as binary data
    binaryData.clear();
    binaryData.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        binaryData.push_back(data[i]);
    }
}

void RequestBody::setEncodedData(const std::map<std::string, std::string> &encodedData) {
    this->encodData = encodedData;
}

// Getters
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

// BINARY-SAFE: New method to get true binary data
const std::vector<char>& RequestBody::getBinaryData() const {
    return binaryData;
}

// BINARY-SAFE: Get binary data as string (safe for binary content)
std::string RequestBody::getBinaryDataAsString() const {
    if (!binaryData.empty()) {
        return std::string(binaryData.begin(), binaryData.end());
    }
    return rawData;
}

std::map<std::string, std::string> RequestBody::getEncodedData() const {
    return encodData;
}

// BINARY-SAFE: Check if data contains null bytes (binary indicator)
bool RequestBody::isBinaryData() const {
    if (binaryData.empty()) return false;
    
    // Check for null bytes in data
    for (size_t i = 0; i < binaryData.size(); ++i) {
        if (binaryData[i] == '\0') {
            return true;
        }
    }
    return false;
}

// BINARY-SAFE: Get actual data size (binary size is authoritative)
size_t RequestBody::getDataSize() const {
    return binaryData.empty() ? rawData.size() : binaryData.size();
}