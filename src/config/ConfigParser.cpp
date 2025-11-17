
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "ParseUtils.hpp"
#include <cstdlib>

ParsingBlock ConfigParser::makeServerBlock(std::vector<std::string>::iterator &tokens, std::vector<std::string>::iterator tokensEnd) {
    ParsingBlock block;
    block.setName("server");
    if (tokens == tokensEnd || *tokens != "server") return block;
    ++tokens;
    if (tokens == tokensEnd || *tokens != "{") {
        throw std::runtime_error("Config parse error: expected '{' after 'server'");
    }
    ++tokens;
    int depth = 1;
    while (tokens != tokensEnd && depth > 0) {
        const std::string &tok = *tokens;
        if (tok == "{") {
            ++depth;
            block.addToTokens(tok);
            ++tokens;
        } else if (tok == "}") {
            --depth;
            if (depth == 0) {
                ++tokens;
                break;
            }
            block.addToTokens(tok);
            ++tokens;
        } else {
            block.addToTokens(tok);
            ++tokens;
        }
    }
    if (depth != 0) {
        throw std::runtime_error("Config parse error: mismatched '{' in server block");
    }
    return block;
}

ParsingBlock ConfigParser::makeLocationBlock(std::vector<std::string>::iterator &tokens) {
    ParsingBlock 	block;

    if (*tokens != "location")
        throw std::runtime_error("Config parse error: expected 'location' token");

    block.setName(*(tokens++));
    while (*tokens != "{") {
        block.addToArgs(*tokens);
        ++tokens;
    }
    if (*tokens != "{")
        throw std::runtime_error("Config parse error: expected '{' after location args");
    ++tokens;
    while (*tokens != "}") {
        block.addToTokens(*tokens);
        ++tokens;
    }
    if (*tokens != "}")
        throw std::runtime_error("Config parse error: expected '}' to close location block");
    ++tokens;
    return (block);
}

LocationConfig		ConfigParser::makeLocationConfig(std::vector<std::string>::iterator &tokens, std::vector<std::string>::iterator tokensEnd) {
	if (tokens == tokensEnd || *tokens != "location") {
		throw std::runtime_error("Config parse error: expected 'location' keyword");
	}

	LocationConfig 	outputLocation;

	tokens++;
	if (tokens == tokensEnd) {
		throw std::runtime_error("Config parse error: unexpected end after 'location'");
	}
	outputLocation.setPath(*(tokens++));
	
	if (tokens == tokensEnd || *tokens != "{") {
		throw std::runtime_error("Config parse error: expected '{' after location path '" + outputLocation.getPath() + "'");
	}
	tokens++;
	
	while (tokens != tokensEnd && *tokens != "}") {
		if (*tokens == "root") {
			tokens++;
			if (tokens == tokensEnd || *tokens == ";" || *tokens == "}") {
				throw std::runtime_error("Config parse error: 'root' directive in location '" + outputLocation.getPath() + "' requires exactly one value");
			}
			std::string rootValue = *tokens;
			tokens++;
			if (tokens != tokensEnd && *tokens != ";" && *tokens != "}" && 
			    *tokens != "index" && *tokens != "methods" && *tokens != "cgi" && 
			    *tokens != "autoindex" && *tokens != "client_max_body_size" && 
			    *tokens != "return" && *tokens != "upload_store") {
				throw std::runtime_error("Config parse error: 'root' directive accepts only one value, found extra: '" + *tokens + "'");
			}
			outputLocation.setRoot(rootValue);
			if (tokens != tokensEnd && *tokens == ";") tokens++;
		}
		else if (*tokens == "index") {
			tokens++;
			if (tokens == tokensEnd || *tokens == ";" || *tokens == "}") {
				throw std::runtime_error("Config parse error: 'index' directive in location '" + outputLocation.getPath() + "' requires exactly one value");
			}
			std::string indexValue = *tokens;
			tokens++;
			if (tokens != tokensEnd && *tokens != ";" && *tokens != "}" && 
			    *tokens != "root" && *tokens != "methods" && *tokens != "cgi" && 
			    *tokens != "autoindex" && *tokens != "client_max_body_size" && 
			    *tokens != "return" && *tokens != "upload_store") {
				throw std::runtime_error("Config parse error: 'index' directive accepts only one value, found extra: '" + *tokens + "'");
			}
			outputLocation.setIndex(indexValue);
			if (tokens != tokensEnd && *tokens == ";") tokens++;
		}
		else if (*tokens == "methods") {
			tokens++;
			if (tokens == tokensEnd || *tokens == ";" || *tokens == "}") {
				throw std::runtime_error("Config parse error: 'methods' directive in location '" + outputLocation.getPath() + "' requires at least one method (GET, POST, or DELETE)");
			}
			int methodCount = 0;
			while (tokens != tokensEnd && (*tokens == "GET" || *tokens == "POST" || *tokens == "DELETE")) {
				outputLocation.setMethods(*tokens);
				tokens++;
				methodCount++;
			}
			if (methodCount == 0) {
				throw std::runtime_error("Config parse error: 'methods' directive in location '" + outputLocation.getPath() + "' requires valid HTTP methods (GET, POST, DELETE)");
			}
			if (tokens != tokensEnd && *tokens == ";") tokens++;
		}
		else if (*tokens == "cgi") {
			tokens++;
			if (tokens == tokensEnd || *tokens == ";" || *tokens == "}") {
				throw std::runtime_error("Config parse error: 'cgi' directive in location '" + outputLocation.getPath() + "' requires exactly one value (on/off)");
			}
			std::string cgiValue = *tokens;
			if (cgiValue != "on" && cgiValue != "off") {
				throw std::runtime_error("Config parse error: 'cgi' value must be 'on' or 'off', got: '" + cgiValue + "'");
			}
			outputLocation.setCGI(cgiValue == "on");
			tokens++;
			if (tokens != tokensEnd && *tokens != ";" && *tokens != "}" && 
			    *tokens != "root" && *tokens != "index" && *tokens != "methods" && 
			    *tokens != "autoindex" && *tokens != "client_max_body_size" && 
			    *tokens != "return" && *tokens != "upload_store") {
				throw std::runtime_error("Config parse error: 'cgi' directive accepts only one value, found extra: '" + *tokens + "'");
			}
			if (tokens != tokensEnd && *tokens == ";") tokens++;
		}
        else if (*tokens == "autoindex") {
			tokens++;
			if (tokens == tokensEnd || *tokens == ";" || *tokens == "}") {
				throw std::runtime_error("Config parse error: 'autoindex' directive in location '" + outputLocation.getPath() + "' requires exactly one value (on/off)");
			}
			std::string autoindexValue = *tokens;
			if (autoindexValue != "on" && autoindexValue != "off") {
				throw std::runtime_error("Config parse error: 'autoindex' value must be 'on' or 'off', got: '" + autoindexValue + "'");
			}
            outputLocation.setAutoIndex(autoindexValue == "on");
			tokens++;
			if (tokens != tokensEnd && *tokens != ";" && *tokens != "}" && 
			    *tokens != "root" && *tokens != "index" && *tokens != "methods" && 
			    *tokens != "cgi" && *tokens != "client_max_body_size" && 
			    *tokens != "return" && *tokens != "upload_store") {
				throw std::runtime_error("Config parse error: 'autoindex' directive accepts only one value, found extra: '" + *tokens + "'");
			}
			if (tokens != tokensEnd && *tokens == ";") tokens++;
        }
        else if (*tokens == "client_max_body_size") {
			tokens++;
			if (tokens == tokensEnd || *tokens == ";" || *tokens == "}") {
				throw std::runtime_error("Config parse error: 'client_max_body_size' directive in location '" + outputLocation.getPath() + "' requires exactly one value");
			}
			std::string sizeValue = *tokens;
			tokens++;
			if (tokens != tokensEnd && *tokens != ";" && *tokens != "}" && 
			    *tokens != "root" && *tokens != "index" && *tokens != "methods" && 
			    *tokens != "cgi" && *tokens != "autoindex" && 
			    *tokens != "return" && *tokens != "upload_store") {
				throw std::runtime_error("Config parse error: 'client_max_body_size' directive accepts only one value, found extra: '" + *tokens + "'");
			}
            try {
                outputLocation.setClientMaxBodySize(ParseUtils::parseMaxBodySize(sizeValue));
            } catch (const std::exception &e) {
                throw std::runtime_error(std::string("Config parse error in location '") + outputLocation.getPath() + "': " + e.what());
            }
			if (tokens != tokensEnd && *tokens == ";") tokens++;
        }
        else if (*tokens == "return") {
            tokens++;
			if (tokens == tokensEnd || *tokens == ";" || *tokens == "}") {
				throw std::runtime_error("Config parse error: 'return' directive in location '" + outputLocation.getPath() + "' requires exactly two values (code and URL)");
			}
			int code = std::atoi((*tokens).c_str());
			if (code != 301 && code != 302 && code != 307) {
				throw std::runtime_error("Config parse error: 'return' code must be 301, 302, or 307, got: " + *tokens);
			}
			tokens++;
			if (tokens == tokensEnd || *tokens == ";" || *tokens == "}") {
				throw std::runtime_error("Config parse error: 'return' directive in location '" + outputLocation.getPath() + "' requires a URL after the status code");
			}
			std::string url = *tokens;
			outputLocation.setReturn(code, url);
			tokens++;
			if (tokens != tokensEnd && *tokens != ";" && *tokens != "}" && 
			    *tokens != "root" && *tokens != "index" && *tokens != "methods" && 
			    *tokens != "cgi" && *tokens != "autoindex" && 
			    *tokens != "client_max_body_size" && *tokens != "upload_store") {
				throw std::runtime_error("Config parse error: 'return' directive accepts only two values, found extra: '" + *tokens + "'");
			}
			if (tokens != tokensEnd && *tokens == ";") tokens++;
        }
        else if (*tokens == "upload_store") {
            tokens++;
			if (tokens == tokensEnd || *tokens == ";" || *tokens == "}") {
				throw std::runtime_error("Config parse error: 'upload_store' directive in location '" + outputLocation.getPath() + "' requires exactly one value (path)");
			}
			std::string uploadPath = *tokens;
            outputLocation.setUploadStore(uploadPath);
			tokens++;
			if (tokens != tokensEnd && *tokens != ";" && *tokens != "}" && 
			    *tokens != "root" && *tokens != "index" && *tokens != "methods" && 
			    *tokens != "cgi" && *tokens != "autoindex" && 
			    *tokens != "client_max_body_size" && *tokens != "return") {
				throw std::runtime_error("Config parse error: 'upload_store' directive accepts only one value, found extra: '" + *tokens + "'");
			}
			if (tokens != tokensEnd && *tokens == ";") tokens++;
        }
		else if (*tokens == ";") {
			tokens++;
		}
		else {
			throw std::runtime_error("Config parse error: unknown directive '" + *tokens + "' in location '" + outputLocation.getPath() + "'");
		}
	}
	
	if (tokens == tokensEnd || *tokens != "}") {
		throw std::runtime_error("Config parse error: expected '}' to close location '" + outputLocation.getPath() + "'");
	}
	tokens++;
	
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

        locations.push_back(makeLocationConfig(it, end));
        continue;
    }
    if (*it == "host") {
        ++it;
        if (it == end || *it == ";") {
            throw std::runtime_error("Config parse error: 'host' directive requires exactly one value");
        }
        std::string hostValue = *it;
        ++it;
        if (it != end && *it != ";" && *it != "location" && *it != "port" && *it != "root" && 
            *it != "autoindex" && *it != "client_max_body_size" && *it != "error_page") {
            throw std::runtime_error("Config parse error: 'host' directive accepts only one value, found extra: '" + *it + "'");
        }
        outputServer.setHost(hostValue);
        if (it != end && *it == ";") ++it;
        continue;
    }
    else if (*it == "port") {
        if (portSet) {
            throw std::runtime_error("Config parse error: duplicate 'port' directive in server block");
        }
        ++it;
        if (it == end || *it == ";") {
            throw std::runtime_error("Config parse error: 'port' directive requires exactly one value");
        }
        int port = ParseUtils::toInt(it);
        ++it;
        if (it != end && *it != ";" && *it != "location" && *it != "host" && *it != "root" && 
            *it != "autoindex" && *it != "client_max_body_size" && *it != "error_page") {
            throw std::runtime_error("Config parse error: 'port' directive accepts only one value, found extra: '" + *it + "'");
        }
        if (port < 0 || port > 65535) {
            throw std::runtime_error("Config parse error: invalid port number (must be 0-65535)");
        }
        outputServer.setPort(port);
        portSet = true;
        if (it != end && *it == ";") ++it;
        continue;
    }
    else if (*it == "root") {
        ++it;
        if (it == end || *it == ";") {
            throw std::runtime_error("Config parse error: 'root' directive requires exactly one value");
        }
        std::string rootValue = *it;
        ++it;
        if (it != end && *it != ";" && *it != "location" && *it != "host" && *it != "port" && 
            *it != "autoindex" && *it != "client_max_body_size" && *it != "error_page") {
            throw std::runtime_error("Config parse error: 'root' directive accepts only one value, found extra: '" + *it + "'");
        }
        outputServer.setRoot(rootValue);
        if (it != end && *it == ";") ++it;
        continue;
    }
    else if (*it == "autoindex") {
        ++it;
        if (it == end || *it == ";") {
            throw std::runtime_error("Config parse error: 'autoindex' directive requires exactly one value (on/off)");
        }
        std::string autoindexValue = *it;
        if (autoindexValue != "on" && autoindexValue != "off") {
            throw std::runtime_error("Config parse error: 'autoindex' value must be 'on' or 'off', got: '" + autoindexValue + "'");
        }
        ++it;
        if (it != end && *it != ";" && *it != "location" && *it != "host" && *it != "port" && 
            *it != "root" && *it != "client_max_body_size" && *it != "error_page") {
            throw std::runtime_error("Config parse error: 'autoindex' directive accepts only one value, found extra: '" + *it + "'");
        }
        outputServer.setAutoIndex(autoindexValue == "on");
        if (it != end && *it == ";") ++it;
        continue;
    }
    else if (*it == "client_max_body_size") {
        ++it;
        if (it == end || *it == ";") {
            throw std::runtime_error("Config parse error: 'client_max_body_size' directive requires exactly one value");
        }
        std::string sizeValue = *it;
        ++it;
        if (it != end && *it != ";" && *it != "location" && *it != "host" && *it != "port" && 
            *it != "root" && *it != "autoindex" && *it != "error_page") {
            throw std::runtime_error("Config parse error: 'client_max_body_size' directive accepts only one value, found extra: '" + *it + "'");
        }
        try {
            outputServer.setClientMaxBodySize(ParseUtils::parseMaxBodySize(sizeValue));
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Config parse error in server block (client_max_body_size): ") + e.what());
        }
        if (it != end && *it == ";") ++it;
        continue;
    }
    else if (*it == "error_page") {
        ++it;
        if (it == end || *it == ";") {
            throw std::runtime_error("Config parse error: 'error_page' directive requires exactly two values (code and path)");
        }
        int errorCode = ParseUtils::toInt(it);
        ++it;
        if (it == end || *it == ";") {
            throw std::runtime_error("Config parse error: 'error_page' directive requires a path after the error code");
        }
        std::string errorPath = *it;
        ++it;
        if (it != end && *it != ";" && *it != "location" && *it != "host" && *it != "port" && 
            *it != "root" && *it != "autoindex" && *it != "client_max_body_size" && *it != "error_page") {
            throw std::runtime_error("Config parse error: 'error_page' directive accepts only two values, found extra: '" + *it + "'");
        }
        errorPages[errorCode] = errorPath;
        if (it != end && *it == ";") ++it;
        continue;
    }
    ++it;
}
	outputServer.setErrorPages(errorPages);
    if (!portSet) {
        throw std::runtime_error("Config parse error: missing 'port' directive in server block");
    }
    if (outputServer.getHost().empty()) {
        outputServer.setHost("0.0.0.0");
    }
    outputServer.setLocations(locations);
	return (outputServer);
}

void ConfigParser::parse(std::string config_file) {
    try {
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
    } catch (const std::exception &e) {
        std::cerr << "Config parse error: " << e.what() << std::endl;
        exit(1);
    }
}

std::vector<ServerConfig> ConfigParser::getServers() const {
	return (this->servers);
}
