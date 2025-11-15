
// ============ include/webserv.hpp ============
#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>


// Forward declarations
class Server;
class eventManager;
class ServerConfig;
class LocationConfig;
class ConfigParser;
class Client;
class Request;
class Response;

enum ConnectionState {
    READING_REQUEST,
    REQUEST_COMPLETE,
    WRITING_RESPONSE,
    REQUEST_ERROR,
    CONNECTION_CLOSED
};

#endif