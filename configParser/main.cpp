#include "ParseUtils.hpp"
#include <vector>
#include <iostream>
#include "ConfigParser.hpp"



void printErrorPages(const std::map<int, std::string> &pages) {
    std::map<int, std::string>::const_iterator it;
    for (it = pages.begin(); it != pages.end(); ++it) {
        std::cout << it->first << " => " << it->second << std::endl;
    }
}


void	printLocations(std::vector<LocationConfig> locations) {
	std::vector<LocationConfig>::iterator it;
	for(it = locations.begin(); it != locations.end(); it++) {
	std::cout << "{" << std::endl;
		std::cout << "\tpath => " << it->getPath() << std::endl;
		std::cout << "\troot => " << it->getRoot() << std::endl;
		std::cout << "\tindex => " << it->getIndex() << std::endl;
		std::cout << "\tCGI is enaled => " << it->isCGIEnabled() << std::endl;
		std::cout << "\tmethods => ";
		std::vector<std::string> m = it->getMethods();
		for(int i = 0; i < m.size(); i++)
			std::cout << m[i] << " ";
		std::cout << std::endl;
	std::cout << "}" << std::endl;
	}
}

void	printServer(ServerConfig &server) {
	std::cout << "port: " << server.getPort() << std::endl;
	std::cout << "host: " << server.getHost() << std::endl;
	std::cout << "root: " << server.getRoot() << std::endl;
	printErrorPages(server.getErrorPages());
	printLocations(server.getLocations());
}

int main() {
	
	ConfigParser WSconfig;

	WSconfig.parse("config.txt");
	std::vector<ServerConfig> servers = WSconfig.getServers();

	for(int i = 0; i < servers.size(); i++) {
		std::cout << "----------------------------------------------------" << std::endl;
		printServer(servers[i]);
	}
		
	return (0);
}