
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "ParseUtils.hpp"
#include <cstdlib>

ParsingBlock ConfigParser::makeServerBlock(std::vector<std::string>::iterator &tokens, std::vector<std::string>::iterator tokensEnd) {
    ParsingBlock block;
    block.setName("server");
    if (tokens == tokensEnd || *tokens != "server") return block;
    ++tokens;
    while (tokens != tokensEnd && *tokens != "{") ++tokens;
    if (tokens == tokensEnd) return block;
    int depth = 0;
    while (tokens != tokensEnd) {
        const std::string &tok = *tokens;
        if (tok == "{") ++depth;
        else if (tok == "}") --depth;
        block.addToTokens(tok);
        ++tokens;
        if (depth == 0) break;
    }
    return block;
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
		throw std::runtime_error("Error: Cannot make LocationConfig - expected 'location' keyword");
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
			if (*(tokens + 1) == "on")
				outputLocation.setCGI(true);
			else if (*(tokens + 1) == "off")
				outputLocation.setCGI(false);
		}
        else if (*tokens == "autoindex") {
            if (*(tokens + 1) == "on")
                outputLocation.setAutoIndex(true);
            else if (*(tokens + 1) == "off")
                outputLocation.setAutoIndex(false);
        }
        else if (*tokens == "client_max_body_size")
            outputLocation.setClientMaxBodySize(ParseUtils::parseMaxBodySize(*(tokens + 1)));
        else if (*tokens == "return") {
            tokens++;
            if (*tokens != "}") {
                int code = std::atoi((*tokens).c_str());
                tokens++;
                if (*tokens != "}") {
                    std::string url = *tokens;
                    outputLocation.setReturn(code, url);
                }
            }
        }
	}
	return (outputLocation);
}

ServerConfig ConfigParser::makeServerConfig(ParsingBlock servBlock) {
    std::vector<std::string>    serverTokens = servBlock.getTokens();
    std::vector<LocationConfig> locations;
    std::map<int, std::string>  errorPages;
    ServerConfig                outputServer;
    bool                        portSet = false;

std::vector<std::string>::iterator it = serverTokens.begin();
std::vector<std::string>::iterator end = serverTokens.end();

while (it != end) {
    if (*it == "location") {
        if ((it + 1) >= end) {
            std::cerr << "ERROR: 'location' token at end of serverTokens\n";
            break;
        }

        locations.push_back(makeLocationConfig(it));
        continue;
    }
    if ((it + 1) < end && *it == "host") {
        outputServer.setHost(*(it + 1));
        it += 2;
        continue;
    }
    else if ((it + 1) < end && *it == "port") {
        if (portSet) {
            throw std::runtime_error("Error: Duplicate 'port' directive in server block - only one port per server block is allowed");
        }
        outputServer.setPort(ParseUtils::toInt(it + 1));
        portSet = true;
        it += 2;
        continue;
    }
    else if ((it + 1) < end && *it == "root") {
        outputServer.setRoot(*(it + 1));
        it += 2;
        continue;
    }
    else if ((it + 1) < end && *it == "autoindex") {
        outputServer.setAutoIndex(*(it + 1) == "on" ? true : false);
        it += 2;
        continue;
    }
    else if ((it + 1) < end && *it == "client_max_body_size") {
        outputServer.setClientMaxBodySize(ParseUtils::parseMaxBodySize(*(it + 1)));
        it += 2;
        continue;
    }
    else if (*it == "error_page") {
        ++it;
        if (it == end) break;
        if ((it + 1) < end) {
            errorPages[ParseUtils::toInt(it)] = *(it + 1);
            it += 2;
            continue;
        } else break;
    }
    ++it;
}
	outputServer.setErrorPages(errorPages);
	outputServer.setLocations(locations);
	return (outputServer);
}

void ConfigParser::parse(std::string config_file) {
    std::vector<std::string> configFileData = ParseUtils::readFile(config_file);
    std::vector<std::string> tokens = ParseUtils::splitAndAccumulate(configFileData);
    std::vector<ParsingBlock> serversBlocks;

    std::vector<std::string>::iterator it = tokens.begin();
    std::vector<std::string>::iterator end = tokens.end();
    while (it != end) {
        if (*it == "server") {
            std::vector<std::string>::iterator before = it;
            serversBlocks.push_back(makeServerBlock(it, end));
            if (it == before) ++it;
            continue;
        }
        ++it;
    }
    for (size_t i = 0; i < serversBlocks.size(); ++i)
        servers.push_back(makeServerConfig(serversBlocks[i]));
}

std::vector<ServerConfig> ConfigParser::getServers() const {
	return (this->servers);
}
