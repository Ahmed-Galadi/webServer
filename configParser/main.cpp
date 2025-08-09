#include "ParseUtils.hpp"
#include <vector>
#include <iostream>
#include "ConfigParser.hpp"


void	printErrorPages(std::map<int, std::string> pages) {

}

void	printLocations(std::vector<LocationConfig> locations) {

}
void	printServer(ServerConfig &server) {
	std::cout << "port: " << server.getPort() << std::endl;
	std::cout << "host: " << server.getHost() << std::endl;
	std::cout << "root: " << server.getRoot() << std::endl;
	// printErrorPages(server.getErrorPages());
	// printLocations(server.getLocations());
}

int main() {
	
	ConfigParser WSconfig;

	WSconfig.parse("config.txt");
	std::vector<ServerConfig> servers = WSconfig.getServers();
	printServer(servers[0]);
	return (0);
}