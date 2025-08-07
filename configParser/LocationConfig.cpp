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

bool	LocationConfig::isCGIEnabled() const {
	return (cgi_enabled);
}