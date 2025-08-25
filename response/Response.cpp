#include "Response.hpp"
#include <sstream>

static const std::map<int, std::string> initExitCodes() {
	std::map<int, std::string> output;
	output[200] = "OK";
	output[201] = "Created";
	output[400] = "Bad Request";
	output[403] = "Forbidden";
	output[404] = "Not Found";
	output[500] = "Internal Server Error";
	return (output);
}

const std::map<int, std::string> Response::EXIT_CODES = initExitCodes();

Response::Response(const Request &req) : request(req) {}

void	Response::setStatus(int statusCode) {
	this->status = statusCode;
}

void	Response::setConnection(const std::string &connect) {
	this->connection = connect;
}

void	Response::setHeaders(const std::map<std::string, std::string> &headers) {
	this->headers = headers;
}

void	Response::setVersion(const std::string &version) {
	this->version = version;
}

void	Response::setBody(const std::string &rawBody) {
	this->body = rawBody;
}

std::string	Response::toString() const {
	std::stringstream stream;

	// version and status line
	stream << version << " " << status << " ";
	std::map<int, std::string>::const_iterator it = EXIT_CODES.find(status);
	if (it != EXIT_CODES.end())
    	stream << it->second << "\r\n";
	else
    	stream << "Unknown Status Code\r\n";
	// headers
		// TO DO
	
	// body
		// TO DO
}