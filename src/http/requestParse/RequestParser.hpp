#pragma once

#include "../../../include/webserv.hpp"
#include "RequestBody.hpp"
#include "Request.hpp"

class RequestParser {
public:
    static void                     boundariesError(std::string &rawBody, const std::string &boundary);
    static void                     extractMultiPart(std::vector<RequestBody> &output, const std::string &bodyRawStr, const std::string &boundary);
    static void                     extractEncodedData(RequestBody &rb, const std::string &bodyRawStr, const std::string &type);
    static std::vector<std::string> splitBody(const std::string &rawBody, const std::string &boundary);
    static void                     extractOctetStream(RequestBody &rb, const std::string &rawData);
    static RequestBody              extractBodyPart(const std::string &rawBodyPart);
    static void                     parseHexa(std::string &hexString);
    static std::vector<RequestBody> ParseBody(const Request &req);
    static std::vector<RequestBody> parseMultipartFormData(const Request& req, const std::string& contentType);
    static std::vector<RequestBody> parseMultipartBinary(const std::vector<char>& data, const std::string& boundary);
    static RequestBody parseMultipartPart(const std::vector<char>& data, size_t start, size_t end);
    static void parsePartHeaders(const std::string& headersStr, RequestBody& body);
    static void parseContentDisposition(const std::string& header, RequestBody& body);
    static void parseContentType(const std::string& header, RequestBody& body);
    static std::string extractBoundary(const std::string& contentType);
    static std::vector<RequestBody> parseUrlEncodedData(const Request& req);
    static std::string urlDecode(const std::string& encoded);
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter);
};