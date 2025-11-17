#include "Response.hpp"
#include <sstream>
#include <ctime>
#include "../../config/ParseUtils.hpp"
#include "../../config/ServerConfig.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>

static const std::map<int, std::string> initExitCodes() {
    std::map<int, std::string> output;
    
    output[200] = "OK";
    output[201] = "Created";
    output[204] = "No Content";
    
    output[301] = "Moved Permanently";
    output[302] = "Found";
    output[304] = "Not Modified";
    
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
    
    output[500] = "Internal Server Error";
    output[501] = "Not Implemented";
    output[502] = "Bad Gateway";
    output[503] = "Service Unavailable";
    output[504] = "Gateway Timeout";
    output[505] = "HTTP Version Not Supported";
    
    return output;
}
const std::map<int, std::string> Response::EXIT_CODES = initExitCodes();

static bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

static std::string readFileContent(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return "";
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

Response* Response::makeErrorResponse(int status, const ServerConfig* serverConfig) {
    Response *outputResponse = new Response();
    std::map<std::string, std::string> headers;
    std::pair<std::string, std::string> tmpPair;
    std::stringstream ss;
    std::string errorBody;
    bool foundErrorPage = false;
    
    outputResponse->setStatus(status);
    outputResponse->setVersion("HTTP/1.0");
    outputResponse->setServer("WebServer/1.0");
    outputResponse->setDate();
    outputResponse->setConnection("close");
    
    if (serverConfig != NULL) {
        std::map<int, std::string> errorPages = serverConfig->getErrorPages();
        std::map<int, std::string>::const_iterator it = errorPages.find(status);
        
        if (it != errorPages.end()) {
            std::string configPath = it->second;
            std::string fullPath;
            
            if (configPath.length() > 0 && configPath[0] == '/') {
                std::string root = serverConfig->getRoot();
                if (root.empty()) {
                    root = "www";
                }
                fullPath = root + configPath;
            } else if (configPath.length() > 1 && configPath[0] == '.' && configPath[1] == '/') {
                fullPath = configPath.substr(2);
            } else {
                std::string root = serverConfig->getRoot();
                if (root.empty()) {
                    root = "www";
                }
                fullPath = root + "/" + configPath;
            }
            
            if (fileExists(fullPath)) {
                errorBody = readFileContent(fullPath);
                if (!errorBody.empty()) {
                    foundErrorPage = true;
                }
            }
        }
    }
    
    if (!foundErrorPage) {
        std::string fileName = "error_pages/" + ParseUtils::toString(status) + ".html";
        if (fileExists(fileName)) {
            errorBody = readFileContent(fileName);
            if (!errorBody.empty()) {
                foundErrorPage = true;
            }
        }
    }
    
    if (!foundErrorPage) {
        std::map<int, std::string>::const_iterator it = EXIT_CODES.find(status);
        std::string statusMessage = (it != EXIT_CODES.end()) ? it->second : "Error";
        
        errorBody = "<html><head><title>" + ParseUtils::toString(status) + " " + statusMessage + "</title></head>";
        errorBody += "<body><h1>" + ParseUtils::toString(status) + " " + statusMessage + "</h1>";
        
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
        } else if (status == 403) {
            errorBody += "<p>You don't have permission to access this resource.</p>";
        } else if (status == 408) {
            errorBody += "<p>The server timed out waiting for the request.</p>";
        } else if (status == 411) {
            errorBody += "<p>Content-Length header is required for this request.</p>";
        } else if (status == 413) {
            errorBody += "<p>The request entity is too large.</p>";
        } else if (status == 415) {
            errorBody += "<p>The media type is not supported by this server.</p>";
        } else if (status == 431) {
            errorBody += "<p>The request header fields are too large.</p>";
        } else if (status == 500) {
            errorBody += "<p>The server encountered an internal error.</p>";
        } else if (status == 501) {
            errorBody += "<p>This method is not implemented by the server.</p>";
        } else if (status == 504) {
            errorBody += "<p>The gateway timed out.</p>";
        } else if (status == 505) {
            errorBody += "<p>The HTTP version is not supported.</p>";
        }
        
        errorBody += "</body></html>";
    }
    
    outputResponse->body = errorBody;
    
    tmpPair.first = "Content-Type";
    tmpPair.second = "text/html";
    headers.insert(tmpPair);
    
    ss << errorBody.size();
    tmpPair.first = "Content-Length";
    tmpPair.second = ss.str();
    headers.insert(tmpPair);
    
    outputResponse->setHeaders(headers);
    
    return outputResponse;
}

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

void Response::addHeader(const std::string &key, const std::string &value)
{
	headers[key] = value;
}


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

std::string	Response::toString() const {
	std::stringstream stream;

	stream << version << " " << status << " ";
	std::map<int, std::string>::const_iterator exitIt = EXIT_CODES.find(status);
	if (exitIt != EXIT_CODES.end())
    	stream << exitIt->second << "\r\n";
	else
    	stream << "Unknown Status Code\r\n";
	std::map<std::string, std::string>::const_iterator headersIter = headers.begin();
	for (; headersIter != headers.end(); headersIter++)
		stream << headersIter->first << ": " << headersIter->second << "\r\n";
	if (!server.empty())
		stream << "Server: " << server << "\r\n";
	if (!date.empty())
		stream << "Date: " << date << "\r\n";
	if (!connection.empty())
		stream << "Connection: " << connection << "\r\n";
	stream << "\r\n";
	if (!body.empty())
		stream << body;
	return (stream.str());
}