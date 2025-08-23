#pragma once

#include <string>
#include <map>
#include "RequestBody.hpp"

class Request {
	private:
		std::map<std::string, std::string>	headers;
		std::string							method;
		std::string							uri;
		std::string							version;
		std::string							body;
		std::map<std::string, std::string>	query;
		bool								isValid;
		bool								isComplete;

		void						parseRawReq(std::string rawRequest);
		std::vector<std::string>	splitHeaderFromBody(std::string rawRequest);
		void						extractRequestData(std::string firstRawLine);
		void						extractHeaders(std::vector<std::string> splitedHeaders);
		void						extractBody(std::string rawBody);
		void						extractQuery(std::string queryString);
	public:
		Request(std::string rawRequest);
		std::map<std::string, std::string>	getHeaders() const;
		std::string							getMethod() const;
		std::string							getURI() const;
		std::string							getVersion() const;
		std::vector<RequestBody>			getBody();
		std::string							getRawBody() const;
		std::map<std::string, std::string>	getQuery() const;
};