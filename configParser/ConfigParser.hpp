#pragma once

#include <vector>
#include "ServerConfig.hpp"

class ConfigParser {
	private:
		std::string 				config_file;
		std::vector<ServerConfig> 	severs;
	public:
		void						parse(std::string config_file);
		std::vector<ServerConfig> 	getServers() const;
};