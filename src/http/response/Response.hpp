#pragma once

#include "../../../include/webserv.hpp"
#include "../requestParse/Request.hpp"

class ServerConfig; // Forward declaration

class	Response {
	private:
		static const std::map<int, std::string> EXIT_CODES;
		std::map<std::string, std::string> 	headers;
		std::string							connection;
		std::string							version;
		std::string							body;
		std::string							server;
		std::string							date;
		int									status;

	public:

		void	setStatus(int statusCode);
		void	setConnection(const std::string &connection);
		void	setHeaders(const std::map<std::string, std::string> &headers);
		void	setVersion(const std::string &versiom);
		void	setBody(const std::string &rawBody);
		void	setServer(const std::string &srv);
		void	setDate();
		void 	addHeader(const std::string &key, const std::string &value);

		std::map<std::string, std::string>	getHeaders() const;
		std::string							getConnection() const;
		std::string							getVersion() const;
		std::string							getBody() const;
		std::string							getServer() const;
		std::string							getDate() const;
		int									getStatus() const;
		std::string 						toString() const;

		static Response *makeErrorResponse(int status, const ServerConfig* serverConfig = NULL); 
};
