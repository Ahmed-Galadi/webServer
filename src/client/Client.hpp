#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../../include/webserv.hpp"
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "../http/requestParse/Request.hpp"
#include "../http/response/Response.hpp"
#include "../http/httpMethods/cgi/CGIhandler.hpp" 

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
    ServerConfig* serverConfig;
    EventManager* eventManager;
    bool waitingForCgi;
public:
    Client(int fd, ServerConfig* serverConfig);
    virtual ~Client();
    void handleRead(EventManager& event_mgr);
    void handleWrite(EventManager& event_mgr);
    void closeConnection(EventManager& event_mgr);
    void parseRequest();
    void buildResponse();
    void setWaitingForCgi(bool waiting) { waitingForCgi = waiting; }
    bool isWaitingForCgi() const { return waitingForCgi; }
    void setCgiResponse(Response* res);
    void setEventManager(EventManager* mgr) { eventManager = mgr; }
    
};

#endif

