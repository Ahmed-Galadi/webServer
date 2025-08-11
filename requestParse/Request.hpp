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

		void						parseRawReq(std::string rawRequest);
		std::vector<std::string>	splitHeaderFromBody(std::string rawRequest);
		void						extractRequestData(std::string firstRawLine);
		void						extractHeaders(std::vector<std::string> splitedHeaders);
		void						extractBody(std::string rawBody);
	public:
		Request(std::string rawRequest);
		std::map<std::string, std::string>	getHeaders() const;
		std::string							getMethod() const;
		std::string							getURI() const;
		std::string							getVersion() const;
		std::string							getBody() const;
		
};
