#include "Request.hpp"
#include <iostream>

void	printRequest(Request r) {
	std::cout << "Method => " << r.getMethod() << std::endl;
	std::cout << "URI => " << r.getURI() << std::endl;
	std::cout << "Version => " << r.getVersion() << std::endl;
	std::cout << "headers {" << std::endl;
	std::map<std::string, std::string> hm = r.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = hm.begin(); it != hm.end(); it++)
		std::cout << "\t" << it->first << "\t:\t" << it->second << std::endl;
	std::cout << "}" << std::endl;
	if (!r.getBody().empty())
		std::cout << "Body [ " << r.getBody() << " ]" << std::endl;
}

int main() {
	std::string request =
    "POST /login HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9\r\n"
    "Accept-Language: en-US,en;q=0.5\r\n"
    "Accept-Encoding: gzip, deflate, br\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 27\r\n"
    "Connection: keep-alive\r\n"
    "Cookie: sessionid=abc123\r\n"
    "\r\n"
    "username=ahmed&password=42";

	Request r(request);
	printRequest(r);

	return (0);
}