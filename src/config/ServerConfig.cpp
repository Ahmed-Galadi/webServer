
#include "ServerConfig.hpp"

ServerConfig::ServerConfig() : client_max_body_size(1024 * 1024), autoindex(false) {}

void	ServerConfig::setPort(int portNum) {
	port = portNum;
}

void	ServerConfig::setRoot(std::string rootStr) {
	root = rootStr;
}

void 	ServerConfig::setHost(const std::string& host) {
    this->host = host;
}

void	ServerConfig::setClientMaxBodySize(size_t size) {
	client_max_body_size = size;
}

void	ServerConfig::setErrorPages(std::map<int, std::string> pages) {
	error_pages = pages;
}

void	ServerConfig::setLocations(std::vector<LocationConfig> locs) {
	locations = locs;
}

void    ServerConfig::setAutoIndex(bool autoindex) {
    this->autoindex = autoindex;
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

size_t	ServerConfig::getClientMaxBodySize() const {
	return (this->client_max_body_size);
}

std::map<int, std::string> ServerConfig::getErrorPages() const {
	return (this->error_pages);
}

std::vector<LocationConfig> ServerConfig::getLocations() const {
	return (this->locations);
}

bool    ServerConfig::getAutoIndex() const {
    return (this->autoindex);
}

const LocationConfig* ServerConfig::findLocation(const std::string& uri) const {
    const LocationConfig* bestMatch = NULL;
    size_t longestMatch = 0;
    
    for (size_t i = 0; i < locations.size(); ++i) {
        std::string locPath = locations[i].getPath();
        
        if (uri.find(locPath) == 0) {
            if (locPath[locPath.length() - 1] == '/') {
                if (uri == locPath || uri.length() > locPath.length()) {
                    if (locPath.length() > longestMatch) {
                        longestMatch = locPath.length();
                        bestMatch = &locations[i];
                    }
                }
            } else {
                if (uri == locPath || 
                    (uri.length() > locPath.length() && 
                     (uri[locPath.length()] == '?' || uri[locPath.length()] == '#'))) {
                    if (locPath.length() > longestMatch) {
                        longestMatch = locPath.length();
                        bestMatch = &locations[i];
                    }
                }
            }
        }
    }
    
    if (!bestMatch) {
        for (size_t i = 0; i < locations.size(); ++i) {
            if (locations[i].getPath() == "/") {
                return &locations[i];
            }
        }
    }
    
    return bestMatch;
}
