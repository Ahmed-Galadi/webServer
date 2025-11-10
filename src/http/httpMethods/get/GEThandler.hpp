#ifndef GETHANDLER_HPP
# define GETHANDLER_HPP

#include "../../response/HttpMethodHandler.hpp"
#include "../utils/MimeType.hpp"
#include "../utils/FileHandler.hpp"
#include <sstream>
#include <iomanip>
#include "webserv.hpp"
#include <map>
#include <dirent.h>

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
    Response* serveTextFile(Response* response, const std::string& filepath, const std::string& mimeType);
    Response* serveBinaryFile(Response* response, const std::string& filepath, const std::string& mimeType);
    Response* createErrorResponse(int statusCode, const std::string& message);
    static bool compareEntries(const DirectoryEntry& a, const DirectoryEntry& b);
    static std::string formatFileSize(off_t bytes);
    static std::string numberToString(size_t number);
};

#endif