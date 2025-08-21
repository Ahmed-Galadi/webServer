
#include "configParser/ParseUtils.hpp"
#include <vector>
#include <iostream>
#include "configParser/ConfigParser.hpp"
#include "requestParse/Request.hpp"

// Prrinting functions

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

void	printRequest(Request r) {
	std::cout << "Method => " << r.getMethod() << std::endl;
	std::cout << "URI => " << r.getURI() << std::endl;
	std::cout << "Version => " << r.getVersion() << std::endl;
	std::cout << "headers {" << std::endl;
	std::map<std::string, std::string> hm = r.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = hm.begin(); it != hm.end(); it++)
		std::cout << "\t" << it->first << "\t:\t" << it->second << std::endl;
	std::cout << "}" << std::endl;
	std::map<std::string, std::string> q = r.getQuery();
	if (!q.empty()) {
	std::cout << "Query {" << std::endl;
		for (std::map<std::string, std::string>::const_iterator it = q.begin(); it != q.end(); it++)
			std::cout << "\t" << it->first << "\t=\t" << it->second << std::endl;
		std::cout << "}" << std::endl;
	}
	std::vector<RequestBody> rb = r.getBody();

	// if (rb.size() == 2)
	// 	std::cout << "WORKINNNGGGG!!!" << std::endl;
	// else
	// 	std::cout << "(T_T)" << std::endl;

	std::cout << "__________________________________[Request Bodies Parsing]______________________________________________\n";
	for (int i = 0; i < rb.size(); i++) {
		std::cout << "{\n";
		std::cout << "Content-Type: " << rb[i].getContentType() << std::endl;
		std::cout << "name: " << rb[i].getName() << std::endl;
		if (!rb[i].getFileName().empty())
			std::cout << "filename: " << rb[i].getFileName() << std::endl;
		std::vector<uint8_t> data = rb[i].getRawData();
		// if (data.empty()) {
		// 	std::cout << "Print-Error: empty data vector!" << std::endl;
		// 	exit(1);
		// }
		std::cout << "data:\t";
		if (r.getHeaders()["Content-Type"] == "application/x-www-form-urlencoded" && r.getBody().size() == 1) {
			std::map<std::string, std::string> encodData = r.getBody()[0].getEncodedData();
			std::map<std::string, std::string>::iterator it;
			std::cout << "\t<===============" << std::endl;
			for(it = encodData.begin(); it != encodData.end(); it++) {
				std::cout << "\t\t\t" << it->first << ": " << it->second << std::endl;
			}
			std::cout << "\t===============>" << std::endl;
		} else {//if (r.getHeaders()["Content-Type"] == "multipart/form-data") {
			for(int x = 0; x < data.size(); x++)
				std::cout << data[x];
			std::cout << std::endl;
			std::cout << "}\n";
		}
	}
	// if (!r.getBody().empty())
	// 	std::cout << "Body [ " << r.getRawBody() << " ]" << std::endl;
}

//-----------------------------------------------------------------

int main() {

	// -------------------------------------[ Config Parse test ]----------------------------------------
	// std::cout << "---------------------------------------[ CONFIG PARSING ]----------------------------------------------" << std::endl;
	// ConfigParser WSconfig;

	// WSconfig.parse("config.txt");
	// std::vector<ServerConfig> servers = WSconfig.getServers();

	// for(int i = 0; i < servers.size(); i++) {
	// 	std::cout << "**************{ server " << (i + 1) << " }****************" << std::endl;
	// 	printServer(servers[i]);
	// }
	// --------------------------------------------------------------------------------------------------

	// -------------------------------------[ Request Parse test ]----------------------------------------
	std::cout << "---------------------------------------[ REQUEST PARSING ]----------------------------------------------" << std::endl;
		// actual raw String Request for [Content-Type: application/x-www-form-urlencoded]
	std::string requestUncoded =
    	"POST /login?user=ahmed&lan=en HTTP/1.0\r\n"
    	"Host: example.com\r\n"
    	"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
    	"Accept: text/html,application/xhtml+xml,application/xml;q=0.9\r\n"
    	"Accept-Language: en-US,en;q=0.5\r\n"
    	"Accept-Encoding: gzip, deflate, br\r\n"
    	"Content-Type: application/x-www-form-urlencoded\r\n"
    	"Content-Length: 27\r\n"
    	"\r\n"
    	"search=C%2B%2B+programming&page=1&filter=title+%26+description";
			
	Request r(requestUncoded);
			// request print of everything including the body
	printRequest(r);

	std::cout << "\r\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n" << std::endl;
		// actual raw String Request for [Content-Type: application/x-www-form-urlencoded]
	std::string requestMultipart = 
    	"POST /upload HTTP/1.0\r\n"
    	"Host: example.com\r\n"
    	"User-Agent: TestAgent/1.0\r\n"
    	"Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    	"Content-Length: 223\r\n"
    	"\r\n"
    	"------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    	"Content-Disposition: form-data; name=\"username\"\r\n"
		"Content-Type: text/plain\r\n"
    	"\r\n"
    	"ahmed\r\n"
    	"------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    	"Content-Disposition: form-data; name=\"file\"; filename=\"hello.txt\"\r\n"
    	"Content-Type: text/plain\r\n"
    	"\r\n"
    	"Hello World!\r\n"
   		"------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";
	Request r1(requestMultipart);
	// request print of everything including the bodies
	printRequest(r1);
	// -----------------------------------------------------------------------------

	return (0);
}

