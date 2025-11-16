#pragma once

#include "../../include/webserv.hpp"
#include "LocationConfig.hpp"
#include "ParseUtils.hpp"

// class ServerConfig;

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
		size_t						client_max_body_size;
		bool						autoindex;
		std::map<int, std::string>	error_pages;
		std::vector<LocationConfig>	locations;
		

	public:
		ServerConfig(); 
		void						setPort(int portNum);
		void						setRoot(std::string root);
		void 						setHost(const std::string& host);
		void						setErrorPages(std::map<int, std::string> pages);
		void						setLocations(std::vector<LocationConfig> locations);
		void 						setClientMaxBodySize(size_t size);
		void						setAutoIndex(bool autoindex);
		
		int							getPort() const;
		std::string					getRoot() const;
		std::string					getHost() const;
		std::map<int, std::string>	getErrorPages() const;
		std::vector<LocationConfig>	getLocations() const;
		size_t						getClientMaxBodySize() const;
		bool						getAutoIndex() const;
		//LocationConfig* findLocation(const std::string& uri) const;
		const LocationConfig* findLocation(const std::string& uri) const;

};