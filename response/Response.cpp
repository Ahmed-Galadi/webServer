#include "Response.hpp"

Response::Response(const std::string &bodyData, int statusCode) : body(bodyData), status(statusCode) {}

std::string	Response::toString() const {
	
}