#pragma once

#include <string>
#include <map>

class Request {
	private:
		std::map<std::string, std::string>	headers;
		std::string							method;
		std::string							uri;
		std::string							version;
		std::string							body;
	
	public:
		std::map<std::string, std::string>	getHeaders() const;
		std::string							getMethod() const;
		std::string							getURI() const;
		std::string							getVersion() const;
		std::string							getBody() const;
		
		void	insertHeader(std::pair<std::string, std::string> header);
		void	setMethod(std::string method);
		void	setURI(std::string URI);
		void	setVersion(std::string version);
		void	setBody(std::string body);
		
};
