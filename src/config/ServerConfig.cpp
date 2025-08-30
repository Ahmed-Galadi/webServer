
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

std::string	ServerConfig::getHost() const {
	return (this->host);
}

std::map<int, std::string> ServerConfig::getErrorPages() const {
	return (this->error_pages);
}

std::vector<LocationConfig> ServerConfig::getLocations() const {
	return (this->locations);
}

LocationConfig* ServerConfig::findLocation(const std::string& uri) const {
    for (size_t i = 0; i < locations.size(); ++i) {
        if (uri.find(locations[i].getPath()) == 0) { // Simple prefix match
            return const_cast<LocationConfig*>(&locations[i]); // Cast to non-const if needed
        }
    }
    return NULL; // No match
}