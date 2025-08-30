#include "LocationConfig.hpp"

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

std::string		LocationConfig::getPath() const {
	return (this->path);
}

std::string		LocationConfig::getRoot() const {
	return (this->root);
}

std::string		LocationConfig::getIndex() const {
	return (this->index);
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