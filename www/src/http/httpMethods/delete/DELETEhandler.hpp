#ifndef DELETEHANDLER_HPP
# define DELETEHANDLER_HPP

#include "../../response/HttpMethodHandler.hpp"
#include "../utils/FileHandler.hpp"
#include "webserv.hpp"
#include <map>

class DELETEhandler : public HttpMethodHandler {
	public:
		Response *handler(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig);
};

#endif