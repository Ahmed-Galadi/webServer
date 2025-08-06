#pragma once

#include <string>
#include <map>
#include <vector>
#include "LocationConfig.hpp"

class ServerConfig {
	private:
		int							port;
		std::string					host;
		std::string					root;
		std::map<int, std::string>	error_pages;
		std::vector<LocationConfig>	locations;
	public:
		int		getPort() const;
		std::string	getRoot() const;
		std::vector<LocationConfig> getLocations() const;
};