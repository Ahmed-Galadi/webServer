#include "Response.hpp"
#include <sstream>
#include <ctime>

// static exit codes
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

// static members

Response	*Response::makeErrorResponse(int status, const std::string &errorBody) {
	Response 							*outputResponse = new Response();
	std::map<std::string, std::string>	headers;
	std::pair<std::string, std::string> tmpPair;
	std::stringstream					ss;

	outputResponse->setStatus(status);
	outputResponse->setVersion("HTTP/1.0");

	// set body
	outputResponse->body = errorBody;
	ss << errorBody.size();
	// set Headers
	tmpPair.first = "Content-Length";
	tmpPair.second = ss.str();
	headers.insert(tmpPair);
	tmpPair.first = "Content-Type";
	tmpPair.second = "text/plain";
	headers.insert(tmpPair);
	outputResponse->setDate();
	return (outputResponse);
}

// setters
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

void	Response::setDate() {
	char dateBuffer[255];
	std::time_t now = std::time(NULL);
	std::tm *gmt = std::gmtime(&now);

	if (!gmt) date = "";
	else std::strftime(dateBuffer, sizeof(dateBuffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
	date = std::string(dateBuffer);
}

void	Response::setServer(const std::string &srv) {
	this->server = srv;
}

// getters
int	Response::getStatus() const {
	return (status);
}

std::string	Response::getConnection() const {
	return (connection);
}

std::map<std::string, std::string>	Response::getHeaders() const {
	return (headers);
}

std::string	Response::getVersion() const {
	return (version);
}

std::string	Response::getBody() const {
	return (body);
}

std::string Response::getServer() const {
	return (server);
}

std::string Response::getDate() const {
	return (date);
}

// to string
std::string	Response::toString() const {
	std::stringstream stream;

	// version and status line
	stream << version << " " << status << " ";
	std::map<int, std::string>::const_iterator exitIt = EXIT_CODES.find(status);
	if (exitIt != EXIT_CODES.end())
    	stream << exitIt->second << "\r\n";
	else
    	stream << "Unknown Status Code\r\n";
	// headers
	std::map<std::string, std::string>::const_iterator headersIter = headers.begin();
	for (; headersIter != headers.end(); headersIter++)
		stream << headersIter->first << ": " << headersIter->second << "\r\n";
	if (!server.empty())
		stream << "Server: " << server << "\r\n";
	if (!date.empty())
		stream << "Date: " << date << "\r\n";
	if (!connection.empty())
		stream << "Connection: " << connection << "\r\n";
	// separator
	stream << "\r\n";
	// body
	if (!body.empty())
		stream << body;
	return (stream.str());
}