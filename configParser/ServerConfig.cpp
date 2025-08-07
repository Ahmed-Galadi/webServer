
#include "ServerConfig.hpp"

void	ServerConfig::setPort(int portNum) {
	port = portNum;
}

void	ServerConfig::setRoot(std::string rootStr) {
	root = rootStr;
}

void	ServerConfig::setHost(std::string hostStr) {
	host = hostStr;
}

void	ServerConfig::setErrorPages(std::map<int, std::string> pages) {
	error_pages = pages;
}

void	ServerConfig::setLocations(std::vector<LocationConfig> locs) {
	locations = locs;
}

int		ServerConfig::getPort() const {
	return (this->port);
}

std::string	ServerConfig::getRoot() const {
	return (this->root);
}

std::vector<LocationConfig> ServerConfig::getLocations() const {
	return (this->locations);
}