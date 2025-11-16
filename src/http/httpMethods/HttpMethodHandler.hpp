#pragma once

#include "../../../include/webserv.hpp"
#include "../requestParse/Request.hpp"
#include "../response/Response.hpp"
#include "../../config/LocationConfig.hpp"
#include "../../config/ServerConfig.hpp"

class HttpMethodHandler {
    public:
        virtual ~HttpMethodHandler();
        virtual Response* handler(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig) = 0;
};
