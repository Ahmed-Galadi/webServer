#pragma once

#include "../requestParse/Request.hpp"
#include "Response.hpp"

class HttpMethodHandler {
	public:
		virtual	~HttpMethodHandler();

		virtual	Response handle(const Request &req) = 0; 
};

class GEThandle :	public HttpMethodHandler {
	public:
		Response handle(const Request &req);
};

class DELETEhandle : public HttpMethodHandler {
	public:
		Response handle(const Request &req);
};

class POSTHandle : public HttpMethodHandler {
	public:
		Response handle(const Request &req);
}
