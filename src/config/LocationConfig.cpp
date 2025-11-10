#include "LocationConfig.hpp"
#include <iostream>

// default 1MB
LocationConfig::LocationConfig() : client_max_body_size(1024 * 1024), autoindex(false), 
                                   has_return(false), return_code(0) {}

void	LocationConfig::setPath(std::string pathStr) {
	path = pathStr;
}

void	LocationConfig::setRoot(std::string rootStr) {
	root = rootStr;
}

void	LocationConfig::setMethods(std::string	method) {
	methods.push_back(method);
}

void	LocationConfig::setIndex(std::string indexStr) {
	index = indexStr;
}

void	LocationConfig::setCGI(bool state) {
	cgi_enabled = state;
}

void	LocationConfig::setAutoIndex(bool autoindex) {
	this->autoindex = autoindex;
}

void	LocationConfig::setClientMaxBodySize(size_t size) {
	client_max_body_size = size;
}

void	LocationConfig::setReturn(int code, const std::string& url) {
	// Validate that return code is one of 301, 302, or 307
	if (code != 301 && code != 302 && code != 307) {
		std::cerr << "[ERROR] Invalid return code " << code 
		          << ". Only 301, 302, and 307 are allowed." << std::endl;
		return;
	}
	has_return = true;
	return_code = code;
	return_url = url;
}

std::string		LocationConfig::getPath() const {
	return (this->path);
}

std::string		LocationConfig::getRoot() const {
	return (this->root);
}

std::string		LocationConfig::getIndex() const {
	return (this->index);
}

bool	LocationConfig::getAutoIndex() const {
	return (this->autoindex);
}

size_t	LocationConfig::getClientMaxBodySize() const {
	return (this->client_max_body_size);
}

std::vector<std::string> LocationConfig::getMethods() const {
	return (this->methods);
}
 
bool	LocationConfig::isCGIEnabled() const {
	return (cgi_enabled);
}

bool LocationConfig::isMethodAllowed(const std::string& method) const {
	for (size_t i = 0; i < methods.size(); ++i) {
		if (methods[i] == method) {
			return true;
		}
	}
	return false;
}

bool	LocationConfig::hasReturn() const {
	return has_return;
}

int		LocationConfig::getReturnCode() const {
	return return_code;
}

std::string	LocationConfig::getReturnUrl() const {
	return return_url;
}