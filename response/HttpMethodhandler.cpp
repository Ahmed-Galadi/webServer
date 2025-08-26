#include "HttpMethodHandler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include "Response.hpp"

static bool isDirectory(const std::string &path) {
	struct stat s;
	if (stat(path.c_str(), &s))
		return (S_ISDIR(s.st_mode));
	return (false);
}

Response	GEThandle::handle(const Request &req) {
	std::string root = "/www"; // config server root
	std::string	path = root + req.getURI();

	// check if it is a directory
	if (isDirectory(path))
		path += "/index.html";
	
	// open file if not open setStatus to 404
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return (Response::notFound());

	// read actual file and gather the content and content length 
	std::ostringstream bodyBuffer;
	bodyBuffer << file.rdbuf();

	std::string responseBody = bodyBuffer.str();
	std::pair<std::string, std::string> contentLength("Content-Length", std::to_string(responseBody.size()));
	std::pair<std::string, std::string> contentType("Content-Type", "text/plain");
	std::map<std::string, std::string> resHeader;
	resHeader.insert(contentLength);
	resHeader.insert(contentType);

	// construct the response
	Response outputRes;

	outputRes.setStatus(200);
	outputRes.setVersion(req.getVersion());
	outputRes.setHeaders(resHeader);
	outputRes.setBody(responseBody);

	return (outputRes);
}

Response	POSThandle::handle(const Request &req) {
	// TO DO
}

Response	DELETEhandle::handle(const Request &req) {
	// TO DO
}


// -----------[Method Response Dispatcher]------------
Response	HttpMethodDispatcher::executeHttpMethod(const Request &request) {
	HttpMethodHandler 	*handler = NULL;

	if (request.getMethod() == "GET") handler = new GEThandle();
	else if (request.getMethod() == "POST") handler = new POSThandle();
	else if (request.getMethod() == "DELETE") handler = new DELETEhandle();
	else {
		std::cout << "ERROR: forbiden method!";
		exit(1);
	}
	Response output = handler->handle(request);
	delete handler;
	return (output);
}