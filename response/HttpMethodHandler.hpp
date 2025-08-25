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

class POSThandle : public HttpMethodHandler {
	public:
		Response handle(const Request &req);
};

class	HttpMethodDispatcher {
	public:
		static Response	executeHttpMethod(const Request &request);
};
