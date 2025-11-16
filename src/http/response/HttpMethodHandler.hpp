#pragma once

#include "../../../include/webserv.hpp"
#include "../requestParse/Request.hpp"
#include "../httpMethods/HttpMethodHandler.hpp"
#include "../../config/ServerConfig.hpp"
#include "Response.hpp"


class	HttpMethodDispatcher {
	public:
		static Response	*executeHttpMethod(const Request &request, 
                                      const ServerConfig& serverConfig);
};
