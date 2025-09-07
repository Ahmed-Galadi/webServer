#include "RequestBody.hpp"
#include <iostream>
#include "../../config/ParseUtils.hpp"
#include "RequestParser.hpp"

// setters
void	RequestBody::setName(const std::string &name) {
	this->name = name;
}

void	RequestBody::setFileName(const std::string &fileName) {
	this->fileName = fileName;
}

void	RequestBody::setContentType(const std::string &contentType) {
	this->contentType = contentType;
}

void	RequestBody::setRawData(const std::string &data) {
	this->rawData = data;
}

void	RequestBody::setEncodedData(const std::map<std::string, std::string> &encodedData) {
	this->encodData = encodedData;
}

// getters
std::string	RequestBody::getName() const {
	return (name);
}

std::string	RequestBody::getFileName() const {
	return (fileName);
}

std::string	RequestBody::getContentType() const {
	return (contentType);
}

std::string	RequestBody::getRawData() const {
	return (rawData);
}

std::map<std::string, std::string> RequestBody::getEncodedData() const {
	return (encodData);
}

