
// ============ Enhanced Debug Version of Client.cpp ============
#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <errno.h>
#include "../http/requestParse/Request.hpp"
#include "../http/response/HttpMethodHandler.hpp"
#include "../http/response/Response.hpp"

Client::Client(int fd, ServerConfig* config) 
    : fd(fd), state(READING_REQUEST), request(NULL), response(NULL),
      bytes_read(0), bytes_written(0), last_activity(time(NULL)), 
      keep_alive(false), config(config) {
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
    ssize_t bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    std::cout << "[DEBUG] recv() returned: " << bytes << " bytes" << std::endl;
    
    last_activity = time(NULL);
    
    if (bytes <= 0)
    {
        if (bytes == 0)
        {
            std::cout << "[DEBUG] Client disconnected (recv returned 0)" << std::endl;
        }
        else
        {
            std::cout << "[DEBUG] Receive error: " << strerror(errno) << " (errno: " << errno << ")" << std::endl;
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
                closeConnection(event_mgr);
                return;
            }
        }
        return;
    }
    
    buffer[bytes] = '\0';
    read_buffer.append(buffer, bytes);
    bytes_read += bytes;
    
    std::cout << "[DEBUG] Received data (" << bytes << " bytes):" << std::endl;
    std::cout << "[DEBUG] Data: [" << std::string(buffer, bytes) << "]" << std::endl;
    std::cout << "[DEBUG] Total buffer size: " << read_buffer.size() << std::endl;
    
    // Try to parse the request
    if (state == READING_REQUEST) {
        std::cout << "[DEBUG] Attempting to parse request..." << std::endl;
        parseRequest();
        std::cout << "[DEBUG] After parsing, state: " << state << std::endl;
    }
    
    // If request is complete, build response
    if (state == REQUEST_COMPLETE)
    {
        std::cout << "[DEBUG] Request complete, building response..." << std::endl;
        buildResponse();
        std::cout << "[DEBUG] Response built, switching to write mode" << std::endl;
        // Modify epoll to wait for write events - PRESERVE the data pointer!
        modifySocketForWrite(event_mgr);
        state = WRITING_RESPONSE;
        std::cout << "[DEBUG] State changed to WRITING_RESPONSE" << std::endl;
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
    
    if (bytes <= 0)
    {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
        {
            std::cerr << "[DEBUG] Send error: " << strerror(errno) << std::endl;
            closeConnection(event_mgr);
        }
        else
        {
            std::cout << "[DEBUG] Send would block, will try again later" << std::endl;
        }
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
        // Try to build a Request object directly from the buffer
        request = new Request(read_buffer);

        // Check for Connection: keep-alive
        std::map<std::string, std::string> headers = request->getHeaders();
        std::map<std::string, std::string>::iterator it = headers.find("Connection");
        if (it != headers.end() && it->second == "keep-alive") {
            keep_alive = true;
            std::cout << "[DEBUG] Keep-alive connection detected" << std::endl;
        } else {
            std::cout << "[DEBUG] Connection will be closed after response" << std::endl;
        }

        state = REQUEST_COMPLETE;
        std::cout << "[DEBUG] Request parsing complete!" << std::endl;
    }
    catch (const Request::IncompleteRequest& e) {
        // Not enough data yet → wait for more
        std::cout << "[DEBUG] Incomplete request, waiting for more data: " << e.what() << std::endl;
        return;
    }
    catch (const std::exception& e) {
        // Any other parsing failure → treat as error
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
    // Create response from request
    response = HttpMethodDispatcher::executeHttpMethod(*request);
    if (keep_alive)
        response->setConnection("keep-alive");
    else
        response->setConnection("close");
    // Convert to raw string for sending
    write_buffer = response->toString();

    std::cout << "[DEBUG] Response built (" << write_buffer.size() << " bytes):" << std::endl;
    std::cout << "[DEBUG] Response: [" << write_buffer << "]" << std::endl;
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
