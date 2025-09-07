#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../../include/webserv.hpp"
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "../http/requestParse/Request.hpp"
#include "../http/response/Response.hpp"
#include "../server/EventManager.hpp"

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
    ServerConfig* config;
    void modifySocketForWrite(EventManager& event_mgr);
    void modifySocketForRead(EventManager& event_mgr);
public:
    Client(int fd, ServerConfig* config);
    virtual ~Client();
    void handleRead(EventManager& event_mgr);
    void handleWrite(EventManager& event_mgr);
    void closeConnection(EventManager& event_mgr);
    void parseRequest();
    void buildResponse();
    bool isTimedOut() const;
    int getFd() const;
    
};

#endif

