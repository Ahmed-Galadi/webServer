#pragma once

#include "../requestParse/Request.hpp"
#include <string>
#include <map>

class	Response {
	private:
		static const std::map<int, std::string> EXIT_CODES;
		Request								request;
		std::map<std::string, std::string> 	headers;
		std::string							connection;
		std::string							version;
		std::string							body;
		int									status;

	public:
		Response(const Request &req);

		void	setStatus(int statusCode);
		void	setConnection(const std::string &connection);
		void	setHeaders(const std::map<std::string, std::string> &headers);
		void	setVersion(const std::string &versiom);
		void	setBody(const std::string &rawBody);

		std::map<std::string, std::string>	getHeaders() const;
		std::string							getConnection() const;
		std::string							getVersion() const;
		std::string							getBody() const;
		std::string							getStatus() const;

		std::string 						toString() const;
};
