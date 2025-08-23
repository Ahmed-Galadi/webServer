#pragma once

#include <string>
#include <map>

class	Response {
	private:
		std::map<std::string, std::string> 	headers;
		std::string							connection;
		std::string							version;
		std::string							body;
		int									status;
	public:
		Response(const std::string &bodyData, int statusCode);

		std::string toString() const;
};
