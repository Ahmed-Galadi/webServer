#pragma once

#include <string>
#include "RequestBody.hpp"
#include "Request.hpp"
#include <vector>

class	RequestParser {
	public:
		static bool						boundariesError(std::string &rawBody, const std::string &boundary);
		static void						extractMultiPart(std::vector<RequestBody> &output, const std::string &bodyRawStr, const std::string &boundary, const std::string &type);
		static void						extractEncodedData(RequestBody &rb, const std::string &bodyRawStr, const std::string &type);
		static std::vector<std::string>	splitBody(const std::string &rawBody, const std::string &boundary);
		static void						extractOctetStream(RequestBody &rb, const std::string &rawData);
		static RequestBody 				extractBodyPart(const std::string &rawBodyPart);
		static void						parseHexa(std::string &hexString);

		static std::vector<RequestBody> ParseBody(const Request &req);
};