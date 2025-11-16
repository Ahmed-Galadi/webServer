#ifndef GETHANDLER_HPP
# define GETHANDLER_HPP

#include "../../../../include/webserv.hpp"
#include "../../response/HttpMethodHandler.hpp"
#include "../utils/MimeType.hpp"
#include "../utils/FileHandler.hpp"

// Move DirectoryEntry outside the class so it can be used with std::vector
struct DirectoryEntry {
    std::string name;
    bool isDir;
    off_t size;
};

class GEThandler : public HttpMethodHandler {
public:
    GEThandler();
    ~GEThandler();

    Response* handler(const Request& req, const LocationConfig* location, const ServerConfig* serverConfig);

private:
    Response* generateDirectoryListing(const std::string& uri,
                                       const std::string& dirPath,
                                       const Request& req);
    Response* createErrorResponse(int statusCode, const std::string& message);
    static bool compareEntries(const DirectoryEntry& a, const DirectoryEntry& b);
    static std::string formatFileSize(off_t bytes);
};

#endif