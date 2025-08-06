
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "ParseUtils.hpp"

void	ConfigParser::parse(std::string config_file) {
	std::vector<std::string>	configFileData = ParseUtils::readFile(config_file);
	std::vector<std::string>	tokens = ParseUtils::splitAndAccumulate(configFileData);

	
}

std::vector<ServerConfig> ConfigParser::getServers() const {
	return (this->severs);
}