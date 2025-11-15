#include "CGIhandler.hpp"
#include "../../../client/Client.hpp"
#include "../../../server/EventManager.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>

// Initialize static members
std::map<int, CgiExecution*> CGIhandler::s_cgiExecutions;
const int CGIhandler::CGI_TIMEOUT_SECONDS;

static std::string stripQueryString(const std::string &uri) {
    std::string::size_type pos = uri.find('?');
    if (pos != std::string::npos)
        return uri.substr(0, pos);
    return uri;
}

static std::string joinPathsNormalize(const std::string &a, const std::string &b) {
    std::string left = a;
    std::string right = b;
    if (!left.empty() && left[left.size()-1] == '/')
        left = left.substr(0, left.size()-1);
    if (!right.empty() && right[0] == '/')
        right = right.substr(1);
    return left + "/" + right;
}

CGIhandler::CGIhandler() {
}

CGIhandler::~CGIhandler() {
}

// Modified handler() - now starts async CGI instead of blocking
Response* CGIhandler::handler(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig) {
    
    // This method is called from Client::buildResponse()
    // It should NOT be used for async CGI - return NULL to indicate async processing
    // The actual async CGI is started via startCgiExecution() called from Client
    (void)req;
    (void)location;
    (void)serverConfig;

    Response* res = new Response();
    res->setStatus(500);
    res->setReasonPhrase("Internal Server Error");
    res->setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
    res->addHeader("Content-Type", "text/html");
    std::ostringstream oss;
    oss << res->getBody().size();
    res->addHeader("Content-Length", oss.str());
    return res;
}

// NEW: Start async CGI execution
bool CGIhandler::startCgiExecution(const Request &req,
                                   const LocationConfig* location,
                                   const ServerConfig* serverConfig,
                                   Client* client,
                                   EventManager& eventMgr) {
    
    // Resolve script path
    std::string uriPath = stripQueryString(req.getURI());
    if (!uriPath.empty() && uriPath[uriPath.size() - 1] == '/')
        uriPath.erase(uriPath.size() - 1);
    uriPath = uriPath.substr(uriPath.find_last_of('/') + 1);
    
    std::string root = location->getRoot();
    if (root.empty()) root = ".";
    
    std::string scriptPath = joinPathsNormalize(root, uriPath);
    
    // Normalize path (remove //)
    std::string normalized;
    for (size_t i = 0; i < scriptPath.size(); ++i) {
        if (i+1 < scriptPath.size() && scriptPath[i] == '/' && scriptPath[i+1] == '/') {
            continue;
        }
        normalized += scriptPath[i];
    }
    scriptPath = normalized;
    
    // Check if path is a directory - directories cannot be executed as CGI scripts
    struct stat pathStat;
    if (stat(scriptPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        Response *res = new Response();
        std::string body = "404 Not Found: CGI script not found or not executable.";
        res->setStatus(404);
        res->setVersion("HTTP/1.0");
        res->setReasonPhrase("Not Found");
        res->setBody(body);
        res->addHeader("Content-Type", "text/plain");
        std::ostringstream oss;
        oss << body.size();
        res->addHeader("Content-Length", oss.str());
        
        client->setCgiResponse(res);
        return false;
    }
    
    // Validate script exists and is executable
    if (access(scriptPath.c_str(), X_OK) != 0) {
        
        Response *res = new Response();
        std::string body = "404 Not Found: CGI script not found or not executable.";
        res->setStatus(404);
        res->setVersion("HTTP/1.0");
        res->setReasonPhrase("Not Found");
        res->setBody(body);
        res->addHeader("Content-Type", "text/plain");
        std::ostringstream oss;
        oss << body.size();
        res->addHeader("Content-Length", oss.str());
        
        client->setCgiResponse(res);
        return false;
    }
    
    // Build environment variables
    CGIhandler tempHandler;
    std::vector<char*> env = tempHandler.buildEnv(req, location, serverConfig);
    
    // Create socketpair for communication
    int socketPair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair) == -1) {
        
        // Free env
        for (size_t i = 0; i < env.size(); ++i) {
            if (env[i]) free(env[i]);
        }
        
        Response *res = new Response();
        res->setStatus(500);
        res->setReasonPhrase("Internal Server Error");
        res->setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        res->addHeader("Content-Type", "text/html");
        std::ostringstream oss;
        oss << res->getBody().size();
        res->addHeader("Content-Length", oss.str());
        
        client->setCgiResponse(res);
        return false;
    }
    
    // Fork and exec CGI script
    pid_t pid;
    if (!forkAndExecCgi(scriptPath, env, socketPair, pid)) {
        
        close(socketPair[0]);
        close(socketPair[1]);
        
        // Free env
        for (size_t i = 0; i < env.size(); ++i) {
            if (env[i]) free(env[i]);
        }
        
        Response *res = new Response();
        res->setStatus(500);
        res->setReasonPhrase("Internal Server Error");
        res->setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        res->addHeader("Content-Type", "text/html");
        std::ostringstream oss;
        oss << res->getBody().size();
        res->addHeader("Content-Length", oss.str());
        
        client->setCgiResponse(res);
        return false;
    }
    
    // Free env (child has its own copy)
    for (size_t i = 0; i < env.size(); ++i) {
        if (env[i]) free(env[i]);
    }
    
    // Close child's end in parent
    close(socketPair[1]);
    
    // Set parent's end to non-blocking
    int flags = fcntl(socketPair[0], F_GETFL, 0);
    if (flags != -1) {
        fcntl(socketPair[0], F_SETFL, flags | O_NONBLOCK);
    }
    
    // Create execution tracker
    CgiExecution* exec = new CgiExecution();
    exec->pid = pid;
    exec->socketFd = socketPair[0];
    exec->client = client;
    exec->serverConfig = serverConfig;
    exec->scriptPath = scriptPath;
    exec->requestBody = std::string(req.getRawBinaryBody().begin(), req.getRawBinaryBody().end());
    exec->bodyBytesWritten = 0;
    exec->startTime = time(NULL);
    exec->state = CGI_WRITING_BODY;
    
    // Add to tracking map
    s_cgiExecutions[socketPair[0]] = exec;
    
    // Add socket to epoll for writing (to send request body)
    eventMgr.addSocket(socketPair[0], exec, EPOLLOUT | EPOLLERR | EPOLLHUP);
    
    // Mark client as waiting for CGI
    client->setWaitingForCgi(true);
    
    
    return true;
}

// NEW: Fork and exec CGI script
bool CGIhandler::forkAndExecCgi(const std::string &scriptPath,
                                const std::vector<char*> &env,
                                int socketPair[2],
                                pid_t& outPid) {
    pid_t pid = fork();
    
    if (pid == -1) {
        return false;
    }
    
    if (pid == 0) {
        // CHILD PROCESS
        
        // Close parent's end
        close(socketPair[0]);
        
        // Redirect stdin/stdout/stderr to socket
        if (dup2(socketPair[1], STDIN_FILENO) == -1 || dup2(socketPair[1], STDOUT_FILENO) == -1 ){
          //  dup2(socketPair[1], STDERR_FILENO) == -1) {
            _exit(127);
        }
        
        close(socketPair[1]);
        
        // MANDATORY: Change to script directory for relative path access
        std::string scriptDir = scriptPath.substr(0, scriptPath.find_last_of('/'));
        if (scriptDir.empty()) scriptDir = ".";
        
        if (chdir(scriptDir.c_str()) == -1) {
            _exit(127);
        }
        
        // Get just the filename for execution
        std::string scriptFilename = scriptPath.substr(scriptPath.find_last_of('/') + 1);
        
        // Prepare argv for execve
        char *argvExec[4];
        argvExec[0] = (char*)"/usr/bin/env";
        argvExec[1] = (char*)"node";
        argvExec[2] = const_cast<char*>(scriptFilename.c_str());
        argvExec[3] = NULL;
        
        // Execute
        execve("/usr/bin/env", argvExec, const_cast<char* const*>(&env[0]));
        
        // If we get here, execve failed
        _exit(127);
    }
    
    // PARENT PROCESS
    outPid = pid;
    return true;
}

// NEW: Handle epoll events for CGI socket
void CGIhandler::handleCgiEvent(int fd, uint32_t events, EventManager& eventMgr) {
    std::map<int, CgiExecution*>::iterator it = s_cgiExecutions.find(fd);
    if (it == s_cgiExecutions.end()) {
        return;
    }
    
    CgiExecution* exec = it->second;
    
    
    // Handle EPOLLERR (true errors)
    if (events & EPOLLERR) {
        exec->state = CGI_ERROR;
        finalizeCgiExecution(exec, eventMgr);
        return;
    }
    
    // Handle EPOLLHUP (hangup - CGI closed connection)
    // This is NORMAL when CGI finishes - try to read remaining data first
    if (events & EPOLLHUP) {
        if (exec->state == CGI_READING_OUTPUT) {
            // Try to read any remaining data
            handleCgiRead(exec, eventMgr);
            
            // Check if handleCgiRead already finalized (it does if EOF is detected)
            // After finalization, the exec is deleted and removed from map
            if (s_cgiExecutions.find(fd) == s_cgiExecutions.end()) {
                return;
            }
        }
        // Mark as complete and finalize
        exec->state = CGI_COMPLETE;
        finalizeCgiExecution(exec, eventMgr);
        return;
    }
    
    // Handle state-specific I/O
    if (exec->state == CGI_WRITING_BODY && (events & EPOLLOUT)) {
        handleCgiWrite(exec, eventMgr);
    } else if (exec->state == CGI_READING_OUTPUT && (events & EPOLLIN)) {
        handleCgiRead(exec, eventMgr);
    }
}

// NEW: Handle writing request body to CGI
void CGIhandler::handleCgiWrite(CgiExecution* exec, EventManager& eventMgr) {
    if (exec->requestBody.empty() || exec->bodyBytesWritten >= exec->requestBody.size()) {
        // Nothing to write or already done
        
        // Shutdown write side to signal EOF to CGI
        shutdown(exec->socketFd, SHUT_WR);
        
        // Switch to reading mode
        exec->state = CGI_READING_OUTPUT;
        eventMgr.modifySocket(exec->socketFd, exec, EPOLLIN | EPOLLERR | EPOLLHUP);
        return;
    }
    
    // Write as much as possible
    size_t remaining = exec->requestBody.size() - exec->bodyBytesWritten;
    ssize_t written = write(exec->socketFd, 
                           exec->requestBody.c_str() + exec->bodyBytesWritten,
                           remaining);
    
    if (written > 0) {
        exec->bodyBytesWritten += written;
        
        // Check if done
        if (exec->bodyBytesWritten >= exec->requestBody.size()) {
            shutdown(exec->socketFd, SHUT_WR);
            exec->state = CGI_READING_OUTPUT;
            eventMgr.modifySocket(exec->socketFd, exec, EPOLLIN | EPOLLERR | EPOLLHUP);
        }
    } else if (written < 0) {
        // Error or would block (EAGAIN/EWOULDBLOCK)
        // In Level-Triggered mode, epoll will notify us again when ready
    }
}

// NEW: Handle reading CGI output
void CGIhandler::handleCgiRead(CgiExecution* exec, EventManager& eventMgr) {
    char buffer[8192];
    ssize_t bytes = read(exec->socketFd, buffer, sizeof(buffer));
    
    if (bytes > 0) {
        exec->output.append(buffer, bytes);
    } else if (bytes == 0) {
        // EOF - CGI finished writing
        exec->state = CGI_COMPLETE;
        finalizeCgiExecution(exec, eventMgr);
    } else {
        // Error or would block
    }
}

// NEW: Finalize CGI execution and send response to client
void CGIhandler::finalizeCgiExecution(CgiExecution* exec, EventManager& eventMgr) {
    
    // Wait for child process
    int status;
    pid_t result = waitpid(exec->pid, &status, WNOHANG);
    
    if (result == 0) {
        // Still running - check if we should wait a bit or kill it
        if (exec->state == CGI_TIMEOUT || exec->state == CGI_ERROR) {
            // Timeout or error - kill it
            kill(exec->pid, SIGKILL);
            waitpid(exec->pid, &status, 0);
            exec->scriptExitCode = -1;
        } else {
            // Normal completion but process hasn't exited yet - wait for it
            result = waitpid(exec->pid, &status, 0); // Block and wait
            if (result > 0 && WIFEXITED(status)) {
                exec->scriptExitCode = WEXITSTATUS(status);
            } else {
                exec->scriptExitCode = -1;
            }
        }
    } else if (result > 0) {
        if (WIFEXITED(status)) {
            exec->scriptExitCode = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            exec->scriptExitCode = -1;
        }
    } else {
        // waitpid failed
        exec->scriptExitCode = -1;
    }
    
    // Build response
    Response* response = NULL;
    
    if (exec->state == CGI_TIMEOUT) {
        // Use standard error response which loads from www/error/504.html
        response = Response::makeErrorResponse(504, exec->serverConfig);
    } else if (exec->state == CGI_ERROR) {
        // Use standard error response which loads from www/error/500.html
        response = Response::makeErrorResponse(500, exec->serverConfig);
    } else if (exec->scriptExitCode != 0) {
        // Script exited with error - use standard error response
        response = Response::makeErrorResponse(500, exec->serverConfig);
    } else {
        // Success - parse CGI output
        CGIhandler tempHandler;
        response = tempHandler.parseCgiOutput(exec->output, exec->scriptExitCode);
    }
    
    // Add Content-Length if missing
    if (response) {
        std::map<std::string, std::string> headers = response->getHeaders();
        if (headers.find("Content-Length") == headers.end()) {
            std::ostringstream oss;
            oss << response->getBody().size();
            response->addHeader("Content-Length", oss.str());
        }
        response->setConnection("close");
    }
    
    // Send response to client
    exec->client->setCgiResponse(response);
    exec->client->setWaitingForCgi(false);
    
    // Cleanup
    cleanupCgiExecution(exec->socketFd, eventMgr);
}

// NEW: Cleanup CGI execution
void CGIhandler::cleanupCgiExecution(int fd, EventManager& eventMgr) {
    std::map<int, CgiExecution*>::iterator it = s_cgiExecutions.find(fd);
    if (it == s_cgiExecutions.end()) {
        return;
    }
    
    CgiExecution* exec = it->second;
    
    
    // Remove from epoll
    eventMgr.removeSocket(fd);
    
    // Close socket
    if (exec->socketFd > 0) {
        close(exec->socketFd);
    }
    
    // Remove from map
    s_cgiExecutions.erase(it);
    
    // Free memory
    delete exec;
}

// NEW: Check for timed out CGI executions
void CGIhandler::checkCgiTimeouts(EventManager& eventMgr) {
    time_t now = time(NULL);
    
    std::map<int, CgiExecution*>::iterator it = s_cgiExecutions.begin();
    while (it != s_cgiExecutions.end()) {
        CgiExecution* exec = it->second;
        
        if (now - exec->startTime > CGI_TIMEOUT_SECONDS) {
            exec->state = CGI_TIMEOUT;  // Use the enum value, not cast from int
            
            // Kill the process
            if (exec->pid > 0) {
                kill(exec->pid, SIGKILL);
            }
            
            // Finalize
            finalizeCgiExecution(exec, eventMgr);
            
            // Iterator invalidated, restart
            it = s_cgiExecutions.begin();
        } else {
            ++it;
        }
    }
}

// Update buildEnv to add PATH_INFO and PATH_TRANSLATED
std::vector<char*> CGIhandler::buildEnv(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig) {
    std::map<std::string, std::string> envMap;
    
    // Copy parent environment (as before)
    const std::map<std::string, std::string>& parentEnv = Server::getEnv();
    const char* importantVars[] = {"PATH", "HOME", "USER", "SHELL", "LANG", "LC_ALL", "LD_LIBRARY_PATH", NULL};
    for (int i = 0; importantVars[i] != NULL; ++i) {
        std::map<std::string, std::string>::const_iterator it = parentEnv.find(importantVars[i]);
        if (it != parentEnv.end()) {
            envMap[it->first] = it->second;
        }
    }
    
    // Enhance PATH (as before - keeping your existing logic)
    if (envMap.find("PATH") != envMap.end()) {
        // ... your existing PATH enhancement code ...
    }
    
    // Extract URI components
    std::string uri = req.getURI();
    size_t queryPos = uri.find('?');
    std::string pathInfo = (queryPos != std::string::npos) ? uri.substr(0, queryPos) : uri;
    std::string queryString = (queryPos != std::string::npos) ? uri.substr(queryPos + 1) : "";
    
    // MANDATORY CGI/1.0 variables
    envMap["REQUEST_METHOD"] = req.getMethod();
    envMap["SCRIPT_NAME"] = pathInfo;
    envMap["SCRIPT_FILENAME"] = location->getRoot() + pathInfo;
    
    // PATH_INFO: The extra path information following the script name
    // For /cgi-bin/script.js/extra/path, PATH_INFO = /extra/path
    std::string scriptPath = stripQueryString(pathInfo);
    if (!scriptPath.empty() && scriptPath[scriptPath.size() - 1] == '/')
        scriptPath.erase(scriptPath.size() - 1);
    std::string scriptName = scriptPath.substr(scriptPath.find_last_of('/') + 1);
    
    // If URI contains more path after script name, that's PATH_INFO
    size_t scriptPos = pathInfo.find(scriptName);
    if (scriptPos != std::string::npos) {
        std::string extraPath = pathInfo.substr(scriptPos + scriptName.length());
        if (!extraPath.empty()) {
            envMap["PATH_INFO"] = extraPath;
            envMap["PATH_TRANSLATED"] = location->getRoot() + extraPath;
        } else {
            envMap["PATH_INFO"] = "";
            envMap["PATH_TRANSLATED"] = "";
        }
    } else {
        envMap["PATH_INFO"] = "";
        envMap["PATH_TRANSLATED"] = "";
    }
    
    envMap["QUERY_STRING"] = queryString;
    envMap["REQUEST_URI"] = uri;
    
    // Content-Length
    std::ostringstream ssContentLen;
    ssContentLen << req.getRawBinaryBody().size();
    envMap["CONTENT_LENGTH"] = ssContentLen.str();
    
    // Standard CGI variables
    envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
    envMap["SERVER_PROTOCOL"] = "HTTP/1.0";
    envMap["REDIRECT_STATUS"] = "200";
    envMap["SERVER_NAME"] = serverConfig->getHost();
    
    std::ostringstream ssPort;
    ssPort << serverConfig->getPort();
    envMap["SERVER_PORT"] = ssPort.str();
    
    envMap["SERVER_SOFTWARE"] = "WebServer/1.0";
    
    // Process headers
    std::map<std::string, std::string> headers = req.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        
        if (it->first == "Content-Type") {
            envMap["CONTENT_TYPE"] = it->second;
            continue;
        }
        if (it->first == "Content-Length") continue;
        
        // Convert to HTTP_* format
        std::string httpKey = "HTTP_";
        for (size_t i = 0; i < it->first.length(); ++i) {
            char c = it->first[i];
            if (c == '-') httpKey += '_';
            else if (c >= 'a' && c <= 'z') httpKey += (char)(c - 'a' + 'A');
            else httpKey += c;
        }
        envMap[httpKey] = it->second;
    }
    
    // Convert to char* vector
    std::vector<char*> env;
    for (std::map<std::string, std::string>::iterator it = envMap.begin();
         it != envMap.end(); ++it) {
        std::string entry = it->first + "=" + it->second;
        char *envEntry = strdup(entry.c_str());
        if (envEntry) env.push_back(envEntry);
    }
    env.push_back(NULL);
    
    return env;
}

// Update parseCgiOutput signature
Response* CGIhandler::parseCgiOutput(const std::string &output, int /*exitCode*/) {
    
    // Find headers/body separator
    size_t headerEnd = output.find("\r\n\r\n");
    size_t bodyStart = 0;
    
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
        bodyStart = (headerEnd != std::string::npos) ? headerEnd + 2 : 0;
    } else {
        bodyStart = headerEnd + 4;
    }
    
    Response *res = new Response();
    res->setVersion("HTTP/1.0");
    
    if (headerEnd > 0) {
        std::string headersStr = output.substr(0, headerEnd);
        std::string bodyStr = (bodyStart < output.size()) ? output.substr(bodyStart) : "";
        
        parseCgiHeaders(headersStr, res);
        
        if (!bodyStr.empty()) {
            res->setBody(bodyStr);
        }
    } else {
        // No headers, treat as all body
        res->setStatus(200);
        res->setReasonPhrase("OK");
        res->setBody(output);
        res->addHeader("Content-Type", "text/html");
    }
    
    if (res->getStatus() == 0) {
        res->setStatus(200);
        res->setReasonPhrase("OK");
    }
    
    return res;
}

void CGIhandler::parseCgiHeaders(const std::string &headersStr, Response *res) {
    std::istringstream iss(headersStr);
    std::string line;
    bool statusSet = false;
    
    while (std::getline(iss, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line = line.substr(0, line.length() - 1);
        }
        
        if (line.empty()) continue;
        
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;
        
        std::string headerName = line.substr(0, colonPos);
        std::string headerValue = line.substr(colonPos + 1);
        
        size_t firstNonSpace = headerValue.find_first_not_of(" \t");
        if (firstNonSpace != std::string::npos) {
            headerValue = headerValue.substr(firstNonSpace);
        }
        
        if (headerName == "Status") {
            std::istringstream statusStream(headerValue);
            int statusCode;
            std::string reasonPhrase;
            if (statusStream >> statusCode) {
                std::getline(statusStream, reasonPhrase);
                if (!reasonPhrase.empty() && reasonPhrase[0] == ' ') {
                    reasonPhrase = reasonPhrase.substr(1);
                }
                res->setStatus(statusCode);
                if (!reasonPhrase.empty()) {
                    res->setReasonPhrase(reasonPhrase);
                }
                statusSet = true;
            }
        } else {
            res->addHeader(headerName, headerValue);
        }
    }
    
    if (!statusSet) {
        res->setStatus(200);
        res->setReasonPhrase("OK");
    }
}