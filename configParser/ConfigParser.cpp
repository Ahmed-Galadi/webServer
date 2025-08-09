
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "ParseUtils.hpp"

ParsingBlock ConfigParser::makeServerBlock(std::vector<std::string>::iterator &tokens, std::vector<std::string>::iterator tokensEnd) {
	ParsingBlock block;
	block.setName("server");
	tokens++;
	while (tokens != tokensEnd && *tokens != "server") {
		block.addToTokens(*tokens);
		tokens++;
	}
	return (block);
}

ParsingBlock ConfigParser::makeLocationBlock(std::vector<std::string>::iterator &tokens) {
	ParsingBlock 	block;
	
	block.setName(*(tokens++));
	for(; *tokens != "{"; tokens++)
		block.addToArgs(*tokens);
	tokens++;
	for(; *tokens != "}"; tokens++)
		block.addToTokens(*tokens);
	return (block);
}

LocationConfig		ConfigParser::makeLocationConfig(std::vector<std::string>::iterator &tokens) {
	if (*tokens != "location") {
		std::cout << "Error: Can't Make LocationConig" << std::endl;
		exit(1);
	}

	LocationConfig 	outputLocation;

	tokens++;
	outputLocation.setPath(*(tokens++));
	tokens++;
	for (; *tokens != "}"; tokens++) {
		if (*tokens == "root")
			outputLocation.setRoot(*(tokens + 1));
		else if (*tokens == "index")
			outputLocation.setIndex(*(tokens + 1));
		else if (*tokens == "methods") {
			tokens++;
			for (; (*tokens == "GET" || *tokens == "POST" || *tokens == "DELETE"); tokens++)
				outputLocation.setMethods(*tokens);
		} else if (*tokens == "cgi") {
			if (*(tokens + 1) == "true")
				outputLocation.setCGI(true);
			else
				outputLocation.setCGI(false);
		}
	}
	return (outputLocation);
}

ServerConfig ConfigParser::makeServerConfig(ParsingBlock servBlock) {
	std::vector<std::string> 	serverTokens = servBlock.getTokens();
	std::vector<LocationConfig>	locations;
	std::map<int, std::string>	errorPages;
	ServerConfig				outputServer;
	int tracker;
	std::vector<std::string>::iterator it;

	for (it = serverTokens.begin(); it != serverTokens.end(); it++) {
		if (*it == "location")
			locations.push_back(makeLocationConfig(it));
		else {
			if (*it == "host")
				outputServer.setHost((*(it + 1)));
			else if (*it == "port")
				outputServer.setPort(ParseUtils::toInt((it + 1)));
			else if (*it == "root")
				outputServer.setRoot(*(it + 1));
			else if (*it == "error_page") {
				for(; it != serverTokens.end(); it++) {
					if (*it == "error_page") {
						it++;
						errorPages[ParseUtils::toInt(it)] = *(it + 1);
					}
				}
			}
		}
	}
	outputServer.setErrorPages(errorPages);
	outputServer.setLocations(locations);
	return (outputServer);
}

void	ConfigParser::parse(std::string config_file) {
	std::vector<std::string>	configFileData = ParseUtils::readFile(config_file);
	std::vector<std::string>	tokens = ParseUtils::splitAndAccumulate(configFileData);
	std::vector<ParsingBlock>	serversBlocks;

	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
		if (*it == "server")
			serversBlocks.push_back(makeServerBlock(it, tokens.end()));

	for (int index = 0; index < serversBlocks.size(); index++)
		servers.push_back(makeServerConfig(serversBlocks[index]));
}

std::vector<ServerConfig> ConfigParser::getServers() const {
	return (this->servers);
}
