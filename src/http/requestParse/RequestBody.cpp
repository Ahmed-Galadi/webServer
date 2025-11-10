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
    std::cout << "[DEBUG] RequestBody::setBinaryData: Input size = " << data.size() << " bytes" << std::endl;
    
    binaryData = data;
    
    // Also create string version for backwards compatibility
    rawData = std::string(data.begin(), data.end());
    
    std::cout << "[DEBUG] RequestBody::setBinaryData: Binary size = " << binaryData.size() << " bytes" << std::endl;
    std::cout << "[DEBUG] RequestBody::setBinaryData: String size = " << rawData.size() << " bytes" << std::endl;
    
    if (binaryData.size() != rawData.size()) {
        std::cout << "[WARNING] String conversion may truncate binary data with null bytes!" << std::endl;
    }
}

// BINARY-SAFE: Enhanced version with explicit size
void RequestBody::setBinaryData(const char* data, size_t size) {
    std::cout << "[DEBUG] RequestBody::setBinaryData: Input size = " << size << " bytes" << std::endl;
    
    binaryData.clear();
    binaryData.reserve(size);
    
    for (size_t i = 0; i < size; ++i) {
        binaryData.push_back(data[i]);
    }
    
    // Create string version - use assign with explicit size to handle null bytes
    rawData.assign(data, size);
    
    std::cout << "[DEBUG] RequestBody::setBinaryData: Binary size = " << binaryData.size() << " bytes" << std::endl;
    std::cout << "[DEBUG] RequestBody::setBinaryData: String size = " << rawData.size() << " bytes" << std::endl;
}

void RequestBody::setRawData(const std::string &data) {
    std::cout << "[DEBUG] RequestBody::setRawData: Input size = " << data.size() << " bytes" << std::endl;
    
    rawData = data;
    
    // Also store as binary data
    binaryData.clear();
    binaryData.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        binaryData.push_back(data[i]);
    }
    
    std::cout << "[DEBUG] RequestBody::setRawData: String size = " << rawData.size() << " bytes" << std::endl;
    std::cout << "[DEBUG] RequestBody::setRawData: Binary size = " << binaryData.size() << " bytes" << std::endl;
    
    // Verify no truncation
    if (rawData.size() != data.size()) {
        std::cout << "[ERROR] setRawData truncated! Input: " << data.size() << ", Stored: " << rawData.size() << std::endl;
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
    // Return string version, but warn if it might be truncated
    if (!binaryData.empty() && rawData.size() != binaryData.size()) {
        std::cout << "[WARNING] getRawData() may be truncated! Use getBinaryData() for binary files" << std::endl;
    }
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