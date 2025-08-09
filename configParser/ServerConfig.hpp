#pragma once

#include <string>
#include <map>
#include <vector>
#include "LocationConfig.hpp"

class ServerConfig {
	private:
	// 	std::vector<std::string> 	serverName;
	//	std::vector<std::string> 	index;
	//	bool							autoIndex;
	//	std::pair<int, std::string> 	return;
	//	size_t						clientBodyMaxSize;
	//	std::vector<std::string>	methods;
		int							port;
		std::string					host;
		std::string					root;
		std::map<int, std::string>	error_pages;
		std::vector<LocationConfig>	locations;

	public:
		void						setPort(int portNum);
		void						setRoot(std::string root);
		void						setHost(std::string host);
		void						setErrorPages(std::map<int, std::string> pages);
		void						setLocations(std::vector<LocationConfig> locations);

		int							getPort() const;
		std::string					getRoot() const;
		std::string					getHost() const;
		std::map<int, std::string>	getErrorPages() const;
		std::vector<LocationConfig>	getLocations() const;
};