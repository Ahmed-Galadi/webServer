#pragma once

#include "../../include/webserv.hpp"
#include "ServerConfig.hpp"
#include "ParsingBlock.hpp"

class ConfigParser {
	private:
		std::string 				config_file;
		std::vector<ServerConfig> 	servers;

		static	ParsingBlock 		makeServerBlock(std::vector<std::string>::iterator &tokens, std::vector<std::string>::iterator tokensEnd);
		static	ParsingBlock 		makeLocationBlock(std::vector<std::string>::iterator &tokens);
		static	LocationConfig		makeLocationConfig(std::vector<std::string>::iterator &tokens, std::vector<std::string>::iterator tokensEnd);
		static	ServerConfig		makeServerConfig(ParsingBlock servBlock);
	public:
		void						parse(std::string config_file);
		std::vector<ServerConfig> 	getServers() const;
};
