
// ============ include/webserv.hpp ============
#ifndef WEBSERV_HPP
#define WEBSERV_HPP

// Standard C++ headers
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <iomanip>
#include <limits>

// Standard C headers
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <cerrno>
#include <cstdio>
#include <cstddef>

// System headers
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>

// Forward declarations
class Server;
class EventManager;
class ServerConfig;
class LocationConfig;
class ConfigParser;
class Client;
class Request;
class Response;
class RequestParser;
class RequestBody;
class HttpMethodHandler;
class GEThandler;
class POSThandler;
class DELETEhandler;
class CGIhandler;
class FileHandler;
class MimeType;
class ParsingBlock;

enum ConnectionState {
    READING_REQUEST,
    REQUEST_COMPLETE,
    WRITING_RESPONSE,
    REQUEST_ERROR,
    CONNECTION_CLOSED
};

#endif