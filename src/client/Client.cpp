
#include "Client.hpp"
#include "../server/EventManager.hpp"
#include "../http/httpMethods/cgi/CGIhandler.hpp"
#include "../http/requestParse/Request.hpp"
#include "../http/response/HttpMethodHandler.hpp"
#include "../http/response/Response.hpp"

static bool isCgiByExtension(const std::string& uri) {
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

void Client::setCgiResponse(Response* res) {
    
    if (response) {
        delete response;
    }
    response = res;
    
    if (response) {
        write_buffer = response->toString();
        
        state = WRITING_RESPONSE;
        
        if (eventManager) {
            eventManager->modifySocket(fd, this, EPOLLOUT | EPOLLERR | EPOLLHUP);
        } else {
            std::cerr << "[ERROR] EventManager not set in client!" << std::endl;
        }
    }
}

void Client::handleRead(EventManager& event_mgr) {

    char buffer[8192];

    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes > 0) {
            last_activity = time(NULL);

            read_buffer.append(buffer, static_cast<size_t>(bytes));
            bytes_read += bytes;
    }

    if (bytes == 0) {
        closeConnection(event_mgr);
        return;
    }

    if (bytes < 0) {
        return;
    }
    last_activity = time(NULL);

    if (time(NULL) - last_activity > 30) {
        std::cout << "[TIMEOUT] Client connection timed out after 30 seconds" << std::endl;
        closeConnection(event_mgr);
        return;
    }

    if (state == READING_REQUEST) {
        parseRequest();
    }

    if (state == REQUEST_COMPLETE) {
        buildResponse();
        
        if (waitingForCgi) {
            return;
        }
        
        event_mgr.modifySocket(fd, this, EPOLLOUT | EPOLLERR | EPOLLHUP);
        state = WRITING_RESPONSE;
    } else if (state == REQUEST_ERROR) {
        if (response) {
            write_buffer = response->toString();
        } else {
            buildResponse();
        }
        event_mgr.modifySocket(fd, this, EPOLLOUT | EPOLLERR | EPOLLHUP);
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
    
    if (bytes < 0)
    {
        return;
    }
    
    if (bytes == 0)
    {
        return;
    }
    
    bytes_written += bytes;
    
    if (bytes_written >= write_buffer.length())
    {
        closeConnection(event_mgr);
    }
}

void Client::closeConnection(EventManager& event_mgr) {
    if (fd > 0) {
        event_mgr.removeSocket(fd);
        close(fd);
        fd = -1;
    }
    state = CONNECTION_CLOSED;
}

void Client::parseRequest() {
    
    try {
        request = new Request(read_buffer);
        
        size_t max_body_size = serverConfig->getClientMaxBodySize();
        size_t actual_body_size = request->getRawBody().size();
        
        if (actual_body_size > max_body_size) {
            
            delete request;
            request = NULL;
            
            response = Response::makeErrorResponse(413, serverConfig);
            if (!response) {
                response = new Response();
                response->setStatus(413);
                response->setBody("<html><body><h1>413 Request Entity Too Large</h1></body></html>");
                response->addHeader("Content-Type", "text/html");
                {
                    std::ostringstream oss;
                    oss << response->getBody().size();
                    response->addHeader("Content-Length", oss.str());
                }
            }
            response->setConnection("close");
            
            state = REQUEST_ERROR;
            return;
        }
        
        
        state = REQUEST_COMPLETE;
    }
    catch (const Request::IncompleteRequest& e) {
        
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
        state = REQUEST_ERROR;
    }
}

void Client::buildResponse() {
    
    if (!request) {
        state = REQUEST_ERROR;
        return;
    }
    
    try {
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
        
        if (location && location->isCGIEnabled() && eventManager && isCgiByExtension(uri)) {
            
            if (CGIhandler::startCgiExecution(*request, location, serverConfig, this, *eventManager)) {
                return;
            }
        }
        
        if (!response) {
            response = HttpMethodDispatcher::executeHttpMethod(*request, *serverConfig);
        }
        
        if (!response) {
            response = Response::makeErrorResponse(500, serverConfig);
        }
        
        response->setConnection("close");
        write_buffer = response->toString();
        
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
