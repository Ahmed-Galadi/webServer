#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../../include/webserv.hpp"
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "../http/requestParse/Request.hpp"
#include "../http/response/Response.hpp"
#include "../server/EventManager.hpp"
#include "../http/httpMethods/cgi/CGIhandler.hpp" 
#include <algorithm>  // for std::transform
#include <cctype>     // for ::tolower
#include <cstdlib>    // for atol
#include <string>     // for std::string


class EventManager;

class Client {
private:
    int fd;
    ConnectionState state;
    Request* request;
    Response* response;
    std::string read_buffer;
    std::string write_buffer;
    size_t bytes_read;
    size_t bytes_written;
    time_t last_activity;
    bool keep_alive;
    ServerConfig* serverConfig;
        // NEW: Store EventManager reference for CGI
    EventManager* eventManager;
     // NEW: CGI tracking
    bool waitingForCgi;  // True if this client is waiting for CGI to complete
    void modifySocketForWrite(EventManager& event_mgr);
    void modifySocketForRead(EventManager& event_mgr);
public:
    Client(int fd, ServerConfig* serverConfig);
    virtual ~Client();
    void handleRead(EventManager& event_mgr);
    void handleWrite(EventManager& event_mgr);
    void closeConnection(EventManager& event_mgr);
    void parseRequest();
    void buildResponse();
    bool isTimedOut() const;
    int getFd() const;
    void setServerConfig( ServerConfig* config);
    const ServerConfig* getServerConfig() const;
        // NEW: CGI-related methods
    void setWaitingForCgi(bool waiting) { waitingForCgi = waiting; }
    bool isWaitingForCgi() const { return waitingForCgi; }
    void setCgiResponse(Response* res);  // Called when CGI completes
        // NEW: Set EventManager
    void setEventManager(EventManager* mgr) { eventManager = mgr; }
    
};

#endif

