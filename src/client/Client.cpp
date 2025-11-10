
// ============ Enhanced Debug Version of Client.cpp ============
#include "Client.hpp"
#include "../http/httpMethods/cgi/CGIhandler.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <errno.h>
#include "../http/requestParse/Request.hpp"
#include "../http/response/HttpMethodHandler.hpp"
#include "../http/response/Response.hpp"

Client::Client(int fd, ServerConfig* serverConfig) 
    : fd(fd), state(READING_REQUEST), request(NULL), response(NULL),
      bytes_read(0), bytes_written(0), last_activity(time(NULL)), 
      keep_alive(false), serverConfig(serverConfig), eventManager(NULL), waitingForCgi(false) {
    if (fd <= 0) throw std::invalid_argument("Invalid file descriptor");
    std::cout << "[DEBUG] Client created with fd: " << fd << std::endl;
}

Client::~Client() {
    std::cout << "[DEBUG] Client destroyed, fd: " << fd << std::endl;
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
    std::cout << "[DEBUG] Setting CGI response for client fd=" << fd << std::endl;
    
    if (response) {
        delete response;
    }
    response = res;
    
    if (response) {
        write_buffer = response->toString();
        std::cout << "[DEBUG] CGI response ready (" << write_buffer.size() << " bytes)" << std::endl;
        
        // IMPORTANT: Change state and modify socket for writing
        state = WRITING_RESPONSE;
        
        // We need EventManager to modify socket - it should be available
        if (eventManager) {
            std::cout << "[DEBUG] Switching to EPOLLOUT for CGI response" << std::endl;
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

// --------------------- [rak nsiti hadi wa9ila] ------------------------
bool Client::isTimedOut() const {
    // TODO: implement timeout logic properly.
    // For now return false just to make the linker happy.
    return false;
}
// -------------- [emplimenti had l9lwa ↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑ ] -----------------------

void Client::handleRead(EventManager& event_mgr) {
    std::cout << "[DEBUG] handleRead called for fd: " << fd << std::endl;

    char buffer[8192];

    // Single recv() call per epoll event to comply with subject requirements
    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
    std::cout << "[DEBUG] recv() returned: " << bytes << " bytes" << std::endl;

    if (bytes > 0) {
            last_activity = time(NULL);

            // Append safely (bytes is positive)
            size_t old_size = read_buffer.size();
            read_buffer.append(buffer, static_cast<size_t>(bytes));
            bytes_read += bytes;
            size_t new_size = read_buffer.size();

            std::cout << "[DEBUG] Buffer size BEFORE append: " << old_size << " bytes" << std::endl;
            std::cout << "[DEBUG] Appending: " << bytes << " bytes" << std::endl;
        std::cout << "[DEBUG] Buffer size AFTER append: " << new_size << " bytes" << std::endl;
        std::cout << "[DEBUG] Expected size after append: " << (old_size + bytes) << std::endl;
        if (new_size != old_size + static_cast<size_t>(bytes)) {
            std::cout << "[ERROR] APPEND FAILED! Expected: " << (old_size + bytes) << ", Got: " << new_size << std::endl;
        }

        // Show first/last bytes for debugging (same as before)
        if (read_buffer.size() > 0) {
            std::cout << "[DEBUG] First 20 bytes: ";
            for (size_t i = 0; i < std::min((size_t)20, read_buffer.size()); ++i)
                printf("%02x ", (unsigned char)read_buffer[i]);
            std::cout << std::endl;

            if (read_buffer.size() > 20) {
                std::cout << "[DEBUG] Last 20 bytes: ";
                size_t start = read_buffer.size() - std::min((size_t)20, read_buffer.size());
                for (size_t i = start; i < read_buffer.size(); ++i)
                    printf("%02x ", (unsigned char)read_buffer[i]);
                std::cout << std::endl;
            }
        }
    }

    // bytes == 0 -> client closed connection
    if (bytes == 0) {
        std::cout << "[DEBUG] Peer closed connection (recv returned 0)" << std::endl;
        closeConnection(event_mgr);
        return;
    }

    // bytes < 0 -> error
    // IMPORTANT: Per subject requirements, we CANNOT check errno after recv()
    // "Checking the value of errno to adjust the server behaviour is strictly forbidden
    // after performing a read or write operation."
    if (bytes < 0) {
        std::cout << "[DEBUG] recv returned error (< 0)" << std::endl;
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
        std::cout << "[DEBUG] Attempting to parse request..." << std::endl;
        parseRequest();
        std::cout << "[DEBUG] After parsing, state: " << state << std::endl;
    }

    if (state == REQUEST_COMPLETE) {
        std::cout << "[DEBUG] Request complete, building response..." << std::endl;
        buildResponse();
        
        // Check if we're waiting for CGI - if so, don't modify socket yet
        if (waitingForCgi) {
            std::cout << "[DEBUG] CGI started, waiting for completion" << std::endl;
            // Don't change state or modify socket - CGI will handle this
            return;
        }
        
        std::cout << "[DEBUG] Response built, switching to write mode" << std::endl;
        modifySocketForWrite(event_mgr);
        state = WRITING_RESPONSE;
        std::cout << "[DEBUG] State changed to WRITING_RESPONSE" << std::endl;
    } else if (state == REQUEST_ERROR) {
        std::cout << "[DEBUG] Request error detected, building error response..." << std::endl;
        if (response) {
            write_buffer = response->toString();
        } else {
            buildResponse();
        }
        modifySocketForWrite(event_mgr);
        state = WRITING_RESPONSE;
        std::cout << "[DEBUG] State changed to WRITING_RESPONSE (error)" << std::endl;
    }
}


void Client::handleWrite(EventManager& event_mgr)
{
    std::cout << "[DEBUG] handleWrite called for fd: " << fd << std::endl;
    std::cout << "[DEBUG] Write buffer size: " << write_buffer.size() << std::endl;
    std::cout << "[DEBUG] Bytes already written: " << bytes_written << std::endl;
    std::cout << "[DEBUG] Current state: " << state << std::endl;
    
    if (write_buffer.empty() || state != WRITING_RESPONSE)
    {
        std::cout << "[DEBUG] Nothing to write or wrong state" << std::endl;
        return;
    }
    
    size_t remaining = write_buffer.length() - bytes_written;
    std::cout << "[DEBUG] Attempting to send " << remaining << " bytes" << std::endl;
    
    ssize_t bytes = send(fd, write_buffer.c_str() + bytes_written, remaining, 0);
    
    std::cout << "[DEBUG] send() returned: " << bytes << std::endl;
    
    last_activity = time(NULL);
    
    // IMPORTANT: Per subject requirements, we CANNOT check errno after send()
    // "Checking the value of errno to adjust the server behaviour is strictly forbidden
    // after performing a read or write operation."
    if (bytes < 0)
    {
        std::cout << "[DEBUG] Send returned error (< 0)" << std::endl;
        // For non-blocking sockets, this may be EAGAIN/EWOULDBLOCK which is normal
        // We simply return and let epoll notify us when socket is ready again
        return;
    }
    
    if (bytes == 0)
    {
        std::cout << "[DEBUG] Send returned 0, connection may be closing" << std::endl;
        return;
    }
    
    bytes_written += bytes;
    std::cout << "[DEBUG] Successfully sent " << bytes << " bytes, total: " << bytes_written << std::endl;
    
    // Check if we've sent the complete response
    if (bytes_written >= write_buffer.length())
    {
        std::cout << "[DEBUG] Complete response sent!" << std::endl;
        if (keep_alive) {
            std::cout << "[DEBUG] Keep-alive connection, resetting for next request" << std::endl;
            // Reset for next request
            state = READING_REQUEST;
            read_buffer.clear();
            write_buffer.clear();
            bytes_read = 0;
            bytes_written = 0;
            if (request)
            {
                delete request;
                request = NULL;
            }
            if (response)
            {
                delete response;
                response = NULL;
            }
            // Switch back to reading mode
            modifySocketForRead(event_mgr);
        }
        else
        {
            std::cout << "[DEBUG] Closing connection (no keep-alive)" << std::endl;
            closeConnection(event_mgr);
        }
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
    std::cout << "[DEBUG] parseRequest: buffer size = " << read_buffer.size() << std::endl;
    
    try {
        // Try to build Request object - it will throw IncompleteRequest if not ready
        request = new Request(read_buffer);
        
        // HTTP/1.0: Validate body size against Content-Length header
        size_t max_body_size = serverConfig->getClientMaxBodySize();
        size_t actual_body_size = request->getRawBody().size();
        
        std::cout << "[DEBUG] Body size: " << actual_body_size 
                  << ", Max allowed: " << max_body_size << std::endl;
        
        if (actual_body_size > max_body_size) {
            std::cout << "[ERROR] Request body exceeds client_max_body_size limit" << std::endl;
            
            // Clean up the request object since it's invalid
            delete request;
            request = NULL;
            
            // Build 413 error response
            response = Response::makeErrorResponse(413);
            if (!response) {
                response = new Response();
                response->setStatus(413);
                response->setReasonPhrase("Request Entity Too Large");
                response->setBody("<html><body><h1>413 Request Entity Too Large</h1></body></html>");
                response->addHeader("Content-Type", "text/html");
                response->addHeader("Content-Length", "70");  // Length of the body above
            }
            response->setConnection("close");
            
            // Set state to indicate we have an error response ready
            state = REQUEST_ERROR;
            return;
        }
        
        // Check for Connection header (HTTP/1.0 compatibility)
        std::map<std::string, std::string> headers = request->getHeaders();
        std::map<std::string, std::string>::iterator it = headers.find("Connection");
        
        if (it != headers.end()) {
            std::string connection = it->second;
            std::transform(connection.begin(), connection.end(), connection.begin(), ::tolower);
            if (connection == "keep-alive") {
                keep_alive = true;
                std::cout << "[DEBUG] Keep-alive connection detected" << std::endl;
            }
        } else {
            // HTTP/1.0 default: close connection after response
            std::cout << "[DEBUG] Connection will be closed after response (HTTP/1.0 default)" << std::endl;
        }
        
        state = REQUEST_COMPLETE;
        std::cout << "[DEBUG] Request parsing complete! Body size: " 
                  << actual_body_size << " bytes" << std::endl;
    }
    catch (const Request::IncompleteRequest& e) {
        // Not enough data yet - wait for more
        std::cout << "[DEBUG] Incomplete request, waiting for more data: " << e.what() << std::endl;
        std::cout << "[DEBUG] Current buffer size: " << read_buffer.size() << " bytes" << std::endl;
        
        // Optional: Check if we're waiting for a body that would exceed the limit
        // This is an optimization to fail early if Content-Length header indicates too large body
        if (request) {
            std::map<std::string, std::string> headers = request->getHeaders();
            std::map<std::string, std::string>::iterator cl_it = headers.find("Content-Length");
            
            if (cl_it != headers.end()) {
                size_t content_length = std::atol(cl_it->second.c_str());
                size_t max_body_size = serverConfig->getClientMaxBodySize();
                
                if (content_length > max_body_size) {
                    std::cout << "[ERROR] Content-Length (" << content_length 
                              << ") exceeds max_body_size (" << max_body_size << ")" << std::endl;
                    
                    delete request;
                    request = NULL;
                    
                    response = Response::makeErrorResponse(413);
                    if (!response) {
                        response = new Response();
                        response->setStatus(413);
                        response->setReasonPhrase("Request Entity Too Large");
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
        std::cout << "[DEBUG] Request parsing failed: " << e.what() << std::endl;
        state = REQUEST_ERROR;
    }
}

void Client::buildResponse() {
    std::cout << "[DEBUG] Building HTTP response..." << std::endl;
    
    if (!request) {
        std::cout << "[DEBUG] Error: no request object" << std::endl;
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
        if (location && location->isCGIEnabled() && eventManager) {
            std::cout << "[DEBUG] CGI request detected for URI: " << uri << std::endl;
            
            if (CGIhandler::startCgiExecution(*request, location, serverConfig, this, *eventManager)) {
                std::cout << "[DEBUG] Waiting for async CGI to complete" << std::endl;
                return;
            }
        }
        
        // Non-CGI or CGI failed
        if (!response) {
            response = HttpMethodDispatcher::executeHttpMethod(*request, *serverConfig);
        }
        
        if (!response) {
            response = Response::makeErrorResponse(500);
        }
        
        response->setConnection("close");
        write_buffer = response->toString();
        
        // Debug output
        if (!write_buffer.empty()) {
            std::cout << "[DEBUG] Final response buffer size: " << write_buffer.size() << " bytes" << std::endl;
            if (write_buffer.size() <= 500) {
                std::cout << "[DEBUG] Response:\n" << write_buffer << std::endl;
            } else {
                std::cout << "[DEBUG] Response (truncated):\n" << write_buffer.substr(0, 500) << "..." << std::endl;
            }
        }
        
    } catch (const std::bad_alloc& e) {
        std::cerr << "[ERROR] Memory allocation failed: " << e.what() << std::endl;
        if (!response) {
            response = Response::makeErrorResponse(500);
            write_buffer = response->toString();
        }
        state = REQUEST_ERROR;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
        if (!response) {
            response = Response::makeErrorResponse(500);
            write_buffer = response->toString();
        }
        state = REQUEST_ERROR;
    } catch (...) {
        std::cout << "[ERROR] Unknown exception caught in buildResponse" << std::endl;
        try {
            response = Response::makeErrorResponse(500);
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
    std::cout << "[DEBUG] Modifying socket for READ events" << std::endl;
    event_mgr.modifySocket(fd, this, EPOLLIN);
}
