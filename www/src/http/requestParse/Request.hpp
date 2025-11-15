#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <map>
#include "RequestBody.hpp"
#include <exception>
#include <algorithm> 

class Request {
public:
    Request(std::string rawRequest);

    // Accessors
    std::string getMethod() const;
    std::string getURI() const;
    std::string getVersion() const;
    std::string getRawBody() const; 
    std::map<std::string, std::string> getHeaders() const;
    std::map<std::string, std::string> getQuery() const;
    std::vector<struct RequestBody> getBody();
    // BINARY-SAFE: New methods
    const std::vector<char>& getRawBinaryBody() const;
    const std::vector<char>& getBinaryBody() const { return binaryBody; };
    void setBinaryBody(const std::vector<char>& data) { binaryBody = data; };
    void extractBinaryBody(const char* data, size_t size);

    // Exceptions
    class InvalidRequest : public std::exception {
        public: const char* what() const throw();
    };
    class IncompleteRequest : public std::exception {
        public: const char* what() const throw();
    };
    class ForbiddenMethod : public std::exception {
        public: const char* what() const throw();
    };
    class NotSupportedRequest : public std::exception {
        public: const char* what() const throw();
    };

private:
    std::string method;
    std::string uri;
    std::string version;
    std::string body;              // String version (may truncate at null bytes)
    std::vector<char> binaryBody;  
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query;

    bool isCompleteRequest(const std::string& rawRequest) const;
    size_t findHeaderEnd(const std::string& rawRequest) const;
    size_t getContentLengthFromHeaders(const std::string& headersSection) const;
    size_t getContentLength() const;

    void parseRawReq(std::string rawRequest);
    void extractRequestData(std::string rawReqData);
    void extractHeaders(std::vector<std::string> splitedHeaders);
    void extractBody(std::string rawBody);
    void extractQuery(std::string queryString);
    std::vector<std::string> splitHeaderFromBody(std::string rawRequest);
};

#endif 
