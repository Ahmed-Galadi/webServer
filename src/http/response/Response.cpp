#include "Response.hpp"
#include <sstream>
#include <ctime>
#include "../../config/ParseUtils.hpp"
#include <fstream>
#include <sstream>

// static exit codes
static const std::map<int, std::string> initExitCodes() {
    std::map<int, std::string> output;
    
    // 2xx Success
    output[200] = "OK";
    output[201] = "Created";
    output[204] = "No Content";
    
    // 3xx Redirection
    output[301] = "Moved Permanently";
    output[302] = "Found";
    output[304] = "Not Modified";
    
    // 4xx Client Error
    output[400] = "Bad Request";
    output[401] = "Unauthorized";
    output[403] = "Forbidden";
    output[404] = "Not Found";
    output[405] = "Method Not Allowed";
    output[408] = "Request Timeout";
    output[409] = "Conflict";
    output[410] = "Gone";
    output[411] = "Length Required";
    output[413] = "Request Entity Too Large";
    output[414] = "Request-URI Too Long";
    output[415] = "Unsupported Media Type";
    
    // 5xx Server Error
    output[500] = "Internal Server Error";
    output[501] = "Not Implemented";
    output[502] = "Bad Gateway";
    output[503] = "Service Unavailable";
    output[504] = "Gateway Timeout";
    output[505] = "HTTP Version Not Supported";
    
    return output;
}
const std::map<int, std::string> Response::EXIT_CODES = initExitCodes();

// static members

// Response.cpp - Updated makeErrorResponse() method

Response* Response::makeErrorResponse(int status) {
    Response *outputResponse = new Response();
    std::map<std::string, std::string> headers;
    std::pair<std::string, std::string> tmpPair;
    std::stringstream ss;
    std::string errorBody;
    
    outputResponse->setStatus(status);
    outputResponse->setVersion("HTTP/1.0");
    outputResponse->setServer("WebServer/1.0");
    outputResponse->setDate();
    outputResponse->setConnection("close");
    
    // Try to read custom error page
    std::string fileName = "www/error/" + ParseUtils::toString(status) + ".html";
    std::ifstream errorFile(fileName.c_str());
    
    if (!errorFile.is_open()) {
        // Fallback to generic error page
        std::map<int, std::string>::const_iterator it = EXIT_CODES.find(status);
        std::string statusMessage = (it != EXIT_CODES.end()) ? it->second : "Error";
        
        errorBody = "<html><head><title>" + ParseUtils::toString(status) + " " + statusMessage + "</title></head>";
        errorBody += "<body><h1>" + ParseUtils::toString(status) + " " + statusMessage + "</h1>";
        
        // Add helpful message based on status code
        if (status == 405) {
			tmpPair.first = "Allow";
    		tmpPair.second = "GET, POST, DELETE";
    		headers.insert(tmpPair);
            errorBody += "<p>The requested method is not allowed for this resource.</p>";
            errorBody += "<p>Allowed methods: GET, POST, DELETE</p>";
        } else if (status == 404) {
            errorBody += "<p>The requested resource was not found on this server.</p>";
        } else if (status == 400) {
            errorBody += "<p>The request could not be understood by the server.</p>";
        } else if (status == 411) {
            errorBody += "<p>Content-Length header is required for this request.</p>";
        } else if (status == 413) {
            errorBody += "<p>The request entity is too large.</p>";
        } else if (status == 415) {
            errorBody += "<p>The media type is not supported by this server.</p>";
        } else if (status == 500) {
            errorBody += "<p>The server encountered an internal error.</p>";
        }
        
        errorBody += "</body></html>";
    } else {
        // Use custom error page
        std::ostringstream buffer;
        buffer << errorFile.rdbuf();
        errorBody = buffer.str();
        errorFile.close();
    }
    
    // Set body
    outputResponse->body = errorBody;
    
    // Set Content-Type and Content-Length headers
    tmpPair.first = "Content-Type";
    tmpPair.second = "text/html";
    headers.insert(tmpPair);
    
    ss << errorBody.size();
    tmpPair.first = "Content-Length";
    tmpPair.second = ss.str();
    headers.insert(tmpPair);
    
    // Add Allow header for 405 Method Not Allowed
    if (status == 405) {
        tmpPair.first = "Allow";
        tmpPair.second = "GET, POST, DELETE";
        headers.insert(tmpPair);
    }
    
    outputResponse->setHeaders(headers);
    
    return outputResponse;
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

void Response::setReasonPhrase(const std::string &phrase)
{
    reasonPhrase = phrase;
}

void Response::addHeader(const std::string &key, const std::string &value)
{
	headers[key] = value;
}


// getters
int	Response::getStatus() const {
	return (status);
}

std::string Response::getReasonPhrase() const
{
     return reasonPhrase;
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