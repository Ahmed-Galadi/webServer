
// ============ Enhanced Debug Version of Client.cpp ============
#include "Client.hpp"
#include "../server/EventManager.hpp"
#include "../http/httpMethods/cgi/CGIhandler.hpp"
#include "../http/requestParse/Request.hpp"
#include "../http/response/HttpMethodHandler.hpp"
#include "../http/response/Response.hpp"

// Helper function to check if URI has a CGI extension
static bool isCgiByExtension(const std::string& uri) {
    // Strip query string first
    std::string uriPath = uri;
    size_t queryPos = uriPath.find('?');
    if (queryPos != std::string::npos) {
        uriPath = uriPath.substr(0, queryPos);
    }
    
    const char* extensions[] = {".php", ".py", ".js", ".rb", ".pl", ".cgi", ".sh", NULL};
    
    for (int i = 0; extensions[i] != NULL; ++i) {
        std::string ext = extensions[i];
        if (uriPath.length() >= ext.length()) {
            if (uriPath.substr(uriPath.length() - ext.length()) == ext) {
                return true;
            }
        }
    }
    return false;
}

Client::Client(int fd, ServerConfig* serverConfig) 
    : fd(fd), state(READING_REQUEST), request(NULL), response(NULL),
      bytes_read(0), bytes_written(0), last_activity(time(NULL)), 
      serverConfig(serverConfig), eventManager(NULL), waitingForCgi(false) {
    if (fd <= 0) throw std::invalid_argument("Invalid file descriptor");
}

Client::~Client() {
    if (fd > 0) close(fd);
    if (request) {
        delete request;
    }
    if (response) {
        delete response;
    }
}

// NEW: Called when CGI completes
void Client::setCgiResponse(Response* res) {
    
    if (response) {
        delete response;
    }
    response = res;
    
    if (response) {
        write_buffer = response->toString();
        
        // IMPORTANT: Change state and modify socket for writing
        state = WRITING_RESPONSE;
        
        // We need EventManager to modify socket - it should be available
        if (eventManager) {
            eventManager->modifySocket(fd, this, EPOLLOUT | EPOLLERR | EPOLLHUP);
        } else {
            std::cerr << "[ERROR] EventManager not set in client!" << std::endl;
        }
    }
}

void Client::setServerConfig(ServerConfig* config) {
    this->serverConfig = config;
}

const ServerConfig* Client::getServerConfig() const {
    return this->serverConfig;
}

bool Client::isTimedOut() const {
    // TODO: implement timeout logic properly.
    // For now return false just to make the linker happy.
    return false;
}

void Client::handleRead(EventManager& event_mgr) {

    char buffer[8192];

    // Single recv() call per epoll event to comply with subject requirements
    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes > 0) {
            last_activity = time(NULL);

            // Append safely (bytes is positive)
            size_t old_size = read_buffer.size();
            read_buffer.append(buffer, static_cast<size_t>(bytes));
            bytes_read += bytes;
            size_t new_size = read_buffer.size();

        if (new_size != old_size + static_cast<size_t>(bytes)) {
        }


    }

    // bytes == 0 -> client closed connection
    if (bytes == 0) {
        closeConnection(event_mgr);
        return;
    }

    // bytes < 0 -> error
    // IMPORTANT: Per subject requirements, we CANNOT check errno after recv()
    // "Checking the value of errno to adjust the server behaviour is strictly forbidden
    // after performing a read or write operation."
    if (bytes < 0) {
        // For non-blocking sockets, negative return is normal when no data is available
        // We don't close the connection, just return and wait for next epoll event
        return;
    }    // update last activity timestamp after draining
    last_activity = time(NULL);

    // Timeout check (keep your logic but make sure it's using per-client last_activity)
    if (time(NULL) - last_activity > 30) {
        std::cout << "[TIMEOUT] Client connection timed out after 30 seconds" << std::endl;
        closeConnection(event_mgr);
        return;
    }

    // Note: use per-client variables for stalled detection, not static ones.
    // Try to parse request now (parseRequest will throw IncompleteRequest if not ready)
    if (state == READING_REQUEST) {
        parseRequest();
    }

    if (state == REQUEST_COMPLETE) {
        buildResponse();
        
        // Check if we're waiting for CGI - if so, don't modify socket yet
        if (waitingForCgi) {
            // Don't change state or modify socket - CGI will handle this
            return;
        }
        
        modifySocketForWrite(event_mgr);
        state = WRITING_RESPONSE;
    } else if (state == REQUEST_ERROR) {
        if (response) {
            write_buffer = response->toString();
        } else {
            buildResponse();
        }
        modifySocketForWrite(event_mgr);
        state = WRITING_RESPONSE;
    }
}


void Client::handleWrite(EventManager& event_mgr)
{
    
    if (write_buffer.empty() || state != WRITING_RESPONSE)
    {
        return;
    }
    
    size_t remaining = write_buffer.length() - bytes_written;
    
    ssize_t bytes = send(fd, write_buffer.c_str() + bytes_written, remaining, 0);
    
    
    last_activity = time(NULL);
    
    // IMPORTANT: Per subject requirements, we CANNOT check errno after send()
    // "Checking the value of errno to adjust the server behaviour is strictly forbidden
    // after performing a read or write operation."
    if (bytes < 0)
    {
        // For non-blocking sockets, this may be EAGAIN/EWOULDBLOCK which is normal
        // We simply return and let epoll notify us when socket is ready again
        return;
    }
    
    if (bytes == 0)
    {
        return;
    }
    
    bytes_written += bytes;
    
    // Check if we've sent the complete response
    if (bytes_written >= write_buffer.length())
    {
        // Always close connection after sending response
        closeConnection(event_mgr);
    }
}

void Client::closeConnection(EventManager& event_mgr) {
    if (fd > 0) {  // Only close valid FDs
        event_mgr.removeSocket(fd);
        close(fd);
        fd = -1;  // Mark as invalid
    }
    state = CONNECTION_CLOSED;
}

void Client::parseRequest() {
    
    try {
        // Try to build Request object - it will throw IncompleteRequest if not ready
        request = new Request(read_buffer);
        
        // HTTP/1.0: Validate body size against Content-Length header
        size_t max_body_size = serverConfig->getClientMaxBodySize();
        size_t actual_body_size = request->getRawBody().size();
        
        if (actual_body_size > max_body_size) {
            
            // Clean up the request object since it's invalid
            delete request;
            request = NULL;
            
            // Build 413 error response
            response = Response::makeErrorResponse(413, serverConfig);
            if (!response) {
                response = new Response();
                response->setStatus(413);
                response->setBody("<html><body><h1>413 Request Entity Too Large</h1></body></html>");
                response->addHeader("Content-Type", "text/html");
                response->addHeader("Content-Length", "70");  // Length of the body above
            }
            response->setConnection("close");
            
            // Set state to indicate we have an error response ready
            state = REQUEST_ERROR;
            return;
        }
        
        // Always use close connection (no keep-alive support)
        
        state = REQUEST_COMPLETE;
    }
    catch (const Request::IncompleteRequest& e) {
        // Not enough data yet - wait for more
        
        // Optional: Check if we're waiting for a body that would exceed the limit
        // This is an optimization to fail early if Content-Length header indicates too large body
        if (request) {
            std::map<std::string, std::string> headers = request->getHeaders();
            std::map<std::string, std::string>::iterator cl_it = headers.find("Content-Length");
            
            if (cl_it != headers.end()) {
                size_t content_length = std::atol(cl_it->second.c_str());
                size_t max_body_size = serverConfig->getClientMaxBodySize();
                
                if (content_length > max_body_size) {
                    delete request;
                    request = NULL;
                    
                    response = Response::makeErrorResponse(413, serverConfig);
                    if (!response) {
                        response = new Response();
                        response->setStatus(413);
                        response->setBody("<html><body><h1>413 Request Entity Too Large</h1></body></html>");
                        response->addHeader("Content-Type", "text/html");
                        std::ostringstream oss;
                        oss << response->getBody().size();
                        response->addHeader("Content-Length", oss.str());
                    }
                    response->setConnection("close");
                    
                    state = REQUEST_ERROR;
                }
            }
        }
        return;
    }
    catch (const std::exception& e) {
        // Any other parsing failure
        state = REQUEST_ERROR;
    }
}

void Client::buildResponse() {
    
    if (!request) {
        state = REQUEST_ERROR;
        return;
    }
    
    try {
        // Find location config
        const LocationConfig* location = NULL;
        const std::vector<LocationConfig>& locations = serverConfig->getLocations();
        
        std::string uri = request->getURI();
        size_t bestMatchLen = 0;
        
        for (size_t i = 0; i < locations.size(); ++i) {
            const std::string& path = locations[i].getPath();
            
            if (uri.find(path) == 0 && path.length() > bestMatchLen) {
                location = &locations[i];
                bestMatchLen = path.length();
            }
        }
        
        // Check if this is a CGI request
        if (location && location->isCGIEnabled() && eventManager && isCgiByExtension(uri)) {
            
            if (CGIhandler::startCgiExecution(*request, location, serverConfig, this, *eventManager)) {
                return;
            }
        }
        
        // Non-CGI or CGI failed
        if (!response) {
            response = HttpMethodDispatcher::executeHttpMethod(*request, *serverConfig);
        }
        
        if (!response) {
            response = Response::makeErrorResponse(500, serverConfig);
        }
        
        response->setConnection("close");
        write_buffer = response->toString();
        
        // Debug output
        if (!write_buffer.empty()) {
            if (write_buffer.size() <= 500) {
            } else {
            }
        }
        
    } catch (const std::bad_alloc& e) {
        std::cerr << "[ERROR] Memory allocation failed: " << e.what() << std::endl;
        if (!response) {
            response = Response::makeErrorResponse(500, serverConfig);
            write_buffer = response->toString();
        }
        state = REQUEST_ERROR;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
        if (!response) {
            response = Response::makeErrorResponse(500, serverConfig);
            write_buffer = response->toString();
        }
        state = REQUEST_ERROR;
    } catch (...) {
        try {
            response = Response::makeErrorResponse(500, serverConfig);
            response->setConnection("close");
            write_buffer = response->toString();
        } catch (...) {
            write_buffer = "HTTP/1.0 500 Internal Server Error\r\n"
                          "Content-Length: 0\r\n"
                          "Connection: close\r\n\r\n";
        }
    }
}


void Client::modifySocketForWrite(EventManager& event_mgr) {
    event_mgr.modifySocket(fd, this, EPOLLOUT);
}



int Client::getFd() const {
    return fd;
}

void Client::modifySocketForRead(EventManager& event_mgr) {
    event_mgr.modifySocket(fd, this, EPOLLIN);
}
