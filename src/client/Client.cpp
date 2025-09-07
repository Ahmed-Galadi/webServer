
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

    // Example: set fields
    // response->setVersion("HTTP/1.0");
    // response->setStatus(200);
    // response->setBody("<html><body><h1>Hello World!</h1></body></html>");

    // Add headers
    // std::map<std::string, std::string> headers;
    // headers["Content-Type"] = "text/html";
    // headers["Content-Length"] = std::to_string(response->getBody().size());
    // headers["Server"] = "MyCppServer/1.0";  // <-- you can later pull from config
    if (keep_alive)
        response->setConnection("keep-alive");
    else
        response->setConnection("close");
    // response->setHeaders(headers);

    // Convert to raw string for sending
    write_buffer = response->toString();

    std::cout << "[DEBUG] Response built (" << write_buffer.size() << " bytes):" << std::endl;
    std::cout << "[DEBUG] Response: [" << write_buffer << "]" << std::endl;
}

// void Client::parseRequest() {
//     std::cout << "[DEBUG] parseRequest: buffer size = " << read_buffer.size() << std::endl;
    
//     // Look for end of HTTP headers (double CRLF)
//     size_t header_end = read_buffer.find("\r\n\r\n");
//     if (header_end == std::string::npos) {
//         std::cout << "[DEBUG] Headers not complete yet (no \\r\\n\\r\\n found)" << std::endl;
//         return;
//     }
    
//     std::cout << "[DEBUG] Found end of headers at position: " << header_end << std::endl;
    
//     // Basic HTTP request parsing
//     std::string headers = read_buffer.substr(0, header_end);
//     std::cout << "[DEBUG] Headers: [" << headers << "]" << std::endl;
    
//     size_t first_line_end = headers.find("\r\n");
//     if (first_line_end == std::string::npos) {
//         std::cout << "[DEBUG] Invalid request: no CRLF in first line" << std::endl;
//         state = REQUEST_ERROR;
//         return;
//     }
    
//     std::string request_line = headers.substr(0, first_line_end);
//     std::cout << "[DEBUG] Request line: [" << request_line << "]" << std::endl;
    
//     // Parse method, URI, and version
//     size_t first_space = request_line.find(' ');
//     size_t second_space = request_line.find(' ', first_space + 1);
    
//     if (first_space == std::string::npos || second_space == std::string::npos) {
//         std::cout << "[DEBUG] Invalid request line format" << std::endl;
//         state = REQUEST_ERROR;
//         return;
//     }
    
//     std::string method = request_line.substr(0, first_space);
//     std::string uri = request_line.substr(first_space + 1, second_space - first_space - 1);
//     std::string version = request_line.substr(second_space + 1);
    
//     std::cout << "[DEBUG] Parsed - Method: [" << method << "], URI: [" << uri << "], Version: [" << version << "]" << std::endl;
    
//     // Create request object (simplified for now)
//     request = new Request();
    
//     // Check for Connection: keep-alive
//     if (headers.find("Connection: keep-alive") != std::string::npos ||
//         headers.find("connection: keep-alive") != std::string::npos) {
//         keep_alive = true;
//         std::cout << "[DEBUG] Keep-alive connection detected" << std::endl;
//     } else {
//         std::cout << "[DEBUG] Connection will be closed after response" << std::endl;
//     }
    
//     state = REQUEST_COMPLETE;
//     std::cout << "[DEBUG] Request parsing complete!" << std::endl;
// }

// void Client::buildResponse() {
//     std::cout << "[DEBUG] Building HTTP response..." << std::endl;
    
//     if (!request) {
//         std::cout << "[DEBUG] Error: no request object" << std::endl;
//         state = REQUEST_ERROR;
//         return;
//     }
    
//     // Create response object
//     response = new Response();
    
//     // Build a simple HTTP response
//     std::string body = "Hello World!";
    
//     write_buffer = "HTTP/1.0 200 OK\r\n";
//     write_buffer += "Content-Type: text/html\r\n";
//     write_buffer += "Content-Length: ";
    
//     // Convert body length to string (C++98 compatible)
//     std::ostringstream oss;
//     oss << body.length();
//     write_buffer += oss.str();
//     write_buffer += "\r\n";
    
//     if (keep_alive) {
//         write_buffer += "Connection: keep-alive\r\n";
//     } else {
//         write_buffer += "Connection: close\r\n";
//     }
//     write_buffer += "\r\n";
//     write_buffer += body;
    
//     std::cout << "[DEBUG] Response built (" << write_buffer.size() << " bytes):" << std::endl;
//     std::cout << "[DEBUG] Response: [" << write_buffer << "]" << std::endl;
// }

// bool Client::isTimedOut() const {
//     const time_t TIMEOUT_SECONDS = 30; // 30 second timeout
//     bool timed_out = (time(NULL) - last_activity) > TIMEOUT_SECONDS;
//     if (timed_out) {
//         std::cout << "[DEBUG] Client fd " << fd << " timed out" << std::endl;
//     }
//     return timed_out;
// }



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