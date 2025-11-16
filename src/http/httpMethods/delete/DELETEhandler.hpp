#ifndef DELETEHANDLER_HPP
# define DELETEHANDLER_HPP

#include "../../../../include/webserv.hpp"
#include "../../response/HttpMethodHandler.hpp"
#include "../utils/FileHandler.hpp"

class DELETEhandler : public HttpMethodHandler {
	public:
		Response *handler(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig);
};

#endif