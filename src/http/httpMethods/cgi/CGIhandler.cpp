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
#include "../../../../include/GlobalUtils.hpp"

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

Response* CGIhandler::handler(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig) {
    
    (void)req;
    (void)location;
    (void)serverConfig;

    Response* res = new Response();
    res->setStatus(500);
    res->setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
    res->addHeader("Content-Type", "text/html");
    std::ostringstream oss;
    oss << res->getBody().size();
    res->addHeader("Content-Length", oss.str());
    return res;
}

bool CGIhandler::startCgiExecution(const Request &req,
                                   const LocationConfig* location,
                                   const ServerConfig* serverConfig,
                                   Client* client,
                                   EventManager& eventMgr) {
    
    std::string uriPath = stripQueryString(req.getURI());
    if (!uriPath.empty() && uriPath[uriPath.size() - 1] == '/')
        uriPath.erase(uriPath.size() - 1);
    uriPath = uriPath.substr(uriPath.find_last_of('/') + 1);
    
    std::string root = location->getRoot();
    if (root.empty()) root = ".";
    
    std::string scriptPath = joinPathsNormalize(root, uriPath);
    
    std::string normalized;
    for (size_t i = 0; i < scriptPath.size(); ++i) {
        if (i+1 < scriptPath.size() && scriptPath[i] == '/' && scriptPath[i+1] == '/') {
            continue;
        }
        normalized += scriptPath[i];
    }
    scriptPath = normalized;
    
    struct stat pathStat;
    if (stat(scriptPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        Response *res = new Response();
        std::string body = "404 Not Found: CGI script not found or not executable.";
        res->setStatus(404);
        res->setVersion("HTTP/1.0");
        res->setBody(body);
        res->addHeader("Content-Type", "text/plain");
        std::ostringstream oss;
        oss << body.size();
        res->addHeader("Content-Length", oss.str());
        
        client->setCgiResponse(res);
        return false;
    }
    
    bool needsExec = true;
    if (scriptPath.size() > 3) {
        std::string ext = scriptPath.substr(scriptPath.size() - 3);
        if (ext == ".py" || ext == ".js") needsExec = false;
    }
    if ((needsExec && access(scriptPath.c_str(), X_OK) != 0) || (!needsExec && access(scriptPath.c_str(), R_OK) != 0)) {
        
        Response *res = new Response();
        std::string body = "404 Not Found: CGI script not found or cannot be executed.";
        res->setStatus(404);
        res->setVersion("HTTP/1.0");
        res->setBody(body);
        res->addHeader("Content-Type", "text/plain");
        std::ostringstream oss;
        oss << body.size();
        res->addHeader("Content-Length", oss.str());
        
        client->setCgiResponse(res);
        return false;
    }
    
    CGIhandler tempHandler;
    std::vector<char*> env = tempHandler.buildEnv(req, location, serverConfig);
    
    int socketPair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair) == -1) {
        
        for (size_t i = 0; i < env.size(); ++i) {
            if (env[i]) free(env[i]);
        }
        
        Response *res = new Response();
        res->setStatus(500);
        res->setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        res->addHeader("Content-Type", "text/html");
        std::ostringstream oss;
        oss << res->getBody().size();
        res->addHeader("Content-Length", oss.str());
        
        client->setCgiResponse(res);
        return false;
    }
    
    pid_t pid;
    if (!forkAndExecCgi(scriptPath, env, socketPair, pid)) {
        
        close(socketPair[0]);
        close(socketPair[1]);
        
        for (size_t i = 0; i < env.size(); ++i) {
            if (env[i]) free(env[i]);
        }
        
        Response *res = new Response();
        res->setStatus(500);
        res->setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        res->addHeader("Content-Type", "text/html");
        std::ostringstream oss;
        oss << res->getBody().size();
        res->addHeader("Content-Length", oss.str());
        
        client->setCgiResponse(res);
        return false;
    }
    
    for (size_t i = 0; i < env.size(); ++i) {
        if (env[i]) free(env[i]);
    }
    
    close(socketPair[1]);
    
    setToNonBlocking(socketPair[0]);
    
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
    
    s_cgiExecutions[socketPair[0]] = exec;

    try {
        eventMgr.addSocket(socketPair[0], exec, EPOLLOUT | EPOLLERR | EPOLLHUP);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to register CGI socket in epoll: " << e.what() << std::endl;
        s_cgiExecutions.erase(socketPair[0]);
        if (exec->socketFd > 0) close(exec->socketFd);
        delete exec;
        return false;
    }
    
    client->setWaitingForCgi(true);
    
    
    return true;
}

bool CGIhandler::forkAndExecCgi(const std::string &scriptPath,
                                const std::vector<char*> &env,
                                int socketPair[2],
                                pid_t& outPid) {
    pid_t pid = fork();
    
    if (pid == -1) {
        return false;
    }
    
    if (pid == 0) {
        
        close(socketPair[0]);
        
        if (dup2(socketPair[1], STDIN_FILENO) == -1 || dup2(socketPair[1], STDOUT_FILENO) == -1 ){
            _exit(127);
        }
        
        close(socketPair[1]);
        
        std::string dir = scriptPath.substr(0, scriptPath.find_last_of('/'));
        if (dir.empty()) dir = ".";
        
        if (chdir(dir.c_str()) != 0) {
            std::cerr << "[ERROR] chdir to " << dir << " failed: " << strerror(errno) << std::endl;
            exit(127);
        }

        std::string scriptFilename = scriptPath.substr(scriptPath.find_last_of('/') + 1);

        if (scriptPath.size() > 3 && scriptPath.substr(scriptPath.size() - 3) == ".py") {
            char* const argv[] = { const_cast<char*>("/usr/bin/python3"), const_cast<char*>(scriptFilename.c_str()), NULL };
            execve("/usr/bin/python3", argv, const_cast<char* const*>(env.data()));
            std::cerr << "[ERROR] execve python3 failed for " << scriptFilename << ": " << strerror(errno) << std::endl;
        } else {
            char* const argv[] = { const_cast<char*>(scriptFilename.c_str()), NULL };
            execve(scriptFilename.c_str(), argv, const_cast<char* const*>(env.data()));
            std::cerr << "[ERROR] execve failed for " << scriptFilename << ": " << strerror(errno) << std::endl;
        }
        
        exit(127);
    }

    outPid = pid;
    return true;
}

void CGIhandler::handleCgiEvent(int fd, uint32_t events, EventManager& eventMgr) {
    std::map<int, CgiExecution*>::iterator it = s_cgiExecutions.find(fd);
    if (it == s_cgiExecutions.end()) {
        return;
    }
    
    CgiExecution* exec = it->second;
    
    
    if (events & EPOLLERR) {
        exec->state = CGI_ERROR;
        finalizeCgiExecution(exec, eventMgr);
        return;
    }
    
    if (events & EPOLLHUP) {
        if (exec->state == CGI_READING_OUTPUT) {
            handleCgiRead(exec, eventMgr);
            
            if (s_cgiExecutions.find(fd) == s_cgiExecutions.end()) {
                return;
            }
        }
        exec->state = CGI_COMPLETE;
        finalizeCgiExecution(exec, eventMgr);
        return;
    }
    
    if (exec->state == CGI_WRITING_BODY && (events & EPOLLOUT)) {
        handleCgiWrite(exec, eventMgr);
    } else if (exec->state == CGI_READING_OUTPUT && (events & EPOLLIN)) {
        handleCgiRead(exec, eventMgr);
    }
}

void CGIhandler::handleCgiWrite(CgiExecution* exec, EventManager& eventMgr) {
    if (exec->requestBody.empty() || exec->bodyBytesWritten >= exec->requestBody.size()) {
        
        shutdown(exec->socketFd, SHUT_WR);
        
        exec->state = CGI_READING_OUTPUT;
        eventMgr.modifySocket(exec->socketFd, exec, EPOLLIN | EPOLLERR | EPOLLHUP);
        return;
    }
    
    size_t remaining = exec->requestBody.size() - exec->bodyBytesWritten;
    ssize_t written = write(exec->socketFd, 
                           exec->requestBody.c_str() + exec->bodyBytesWritten,
                           remaining);
    
    if (written > 0) {
        exec->bodyBytesWritten += written;
        
        if (exec->bodyBytesWritten >= exec->requestBody.size()) {
            shutdown(exec->socketFd, SHUT_WR);
            exec->state = CGI_READING_OUTPUT;
            eventMgr.modifySocket(exec->socketFd, exec, EPOLLIN | EPOLLERR | EPOLLHUP);
        }
    } else if (written < 0) {
    }
}

void CGIhandler::handleCgiRead(CgiExecution* exec, EventManager& eventMgr) {
    char buffer[8192];
    ssize_t bytes = read(exec->socketFd, buffer, sizeof(buffer));
    
    if (bytes > 0) {
        exec->output.append(buffer, bytes);
    } else if (bytes == 0) {
        exec->state = CGI_COMPLETE;
        finalizeCgiExecution(exec, eventMgr);
    } else {
    }
}

void CGIhandler::finalizeCgiExecution(CgiExecution* exec, EventManager& eventMgr) {
    
    int status;
    pid_t result = waitpid(exec->pid, &status, WNOHANG);
    
    if (result == 0) {
        if (exec->state == CGI_TIMEOUT || exec->state == CGI_ERROR) {
            kill(exec->pid, SIGKILL);
            waitpid(exec->pid, &status, 0);
            exec->scriptExitCode = -1;
        } else {
            result = waitpid(exec->pid, &status, 0);
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
        exec->scriptExitCode = -1;
    }
    
    Response* response = NULL;
    
    if (exec->state == CGI_TIMEOUT) {
        response = Response::makeErrorResponse(504, exec->serverConfig);
    } else if (exec->state == CGI_ERROR) {
        response = Response::makeErrorResponse(500, exec->serverConfig);
    } else if (exec->scriptExitCode != 0) {
        response = Response::makeErrorResponse(500, exec->serverConfig);
    } else {
        CGIhandler tempHandler;
        response = tempHandler.parseCgiOutput(exec->output, exec->scriptExitCode);
    }
    
    if (response) {
        std::map<std::string, std::string> headers = response->getHeaders();
        if (headers.find("Content-Length") == headers.end()) {
            std::ostringstream oss;
            oss << response->getBody().size();
            response->addHeader("Content-Length", oss.str());
        }
        response->setConnection("close");
    }
    
    exec->client->setCgiResponse(response);
    exec->client->setWaitingForCgi(false);
    
    cleanupCgiExecution(exec->socketFd, eventMgr);
}

void CGIhandler::cleanupCgiExecution(int fd, EventManager& eventMgr) {
    std::map<int, CgiExecution*>::iterator it = s_cgiExecutions.find(fd);
    if (it == s_cgiExecutions.end()) {
        return;
    }
    
    CgiExecution* exec = it->second;
    
    
    eventMgr.removeSocket(fd);
    
    if (exec->socketFd > 0) {
        close(exec->socketFd);
    }
    
    s_cgiExecutions.erase(it);
    
    delete exec;
}

void CGIhandler::checkCgiTimeouts(EventManager& eventMgr) {
    time_t now = time(NULL);
    
    std::map<int, CgiExecution*>::iterator it = s_cgiExecutions.begin();
    while (it != s_cgiExecutions.end()) {
        CgiExecution* exec = it->second;
        
        if (now - exec->startTime > CGI_TIMEOUT_SECONDS) {
            exec->state = CGI_TIMEOUT;
            
            if (exec->pid > 0) {
                kill(exec->pid, SIGKILL);
            }
            
            finalizeCgiExecution(exec, eventMgr);
            
            it = s_cgiExecutions.begin();
        } else {
            ++it;
        }
    }
}

std::vector<char*> CGIhandler::buildEnv(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig) {
    std::map<std::string, std::string> envMap;
    
    const std::map<std::string, std::string>& parentEnv = Server::getEnv();
    const char* importantVars[] = {"PATH", "HOME", "USER", "SHELL", "LANG", "LC_ALL", "LD_LIBRARY_PATH", NULL};
    for (int i = 0; importantVars[i] != NULL; ++i) {
        std::map<std::string, std::string>::const_iterator it = parentEnv.find(importantVars[i]);
        if (it != parentEnv.end()) {
            envMap[it->first] = it->second;
        }
    }
    
    if (envMap.find("PATH") != envMap.end()) {
    }
    
    std::string uri = req.getURI();
    size_t queryPos = uri.find('?');
    std::string pathInfo = (queryPos != std::string::npos) ? uri.substr(0, queryPos) : uri;
    std::string queryString = (queryPos != std::string::npos) ? uri.substr(queryPos + 1) : "";
    
    envMap["REQUEST_METHOD"] = req.getMethod();
    envMap["SCRIPT_NAME"] = pathInfo;
    envMap["SCRIPT_FILENAME"] = location->getRoot() + pathInfo;
    
    std::string scriptPath = stripQueryString(pathInfo);
    if (!scriptPath.empty() && scriptPath[scriptPath.size() - 1] == '/')
        scriptPath.erase(scriptPath.size() - 1);
    std::string scriptName = scriptPath.substr(scriptPath.find_last_of('/') + 1);
    
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
    
    std::ostringstream ssContentLen;
    ssContentLen << req.getRawBinaryBody().size();
    envMap["CONTENT_LENGTH"] = ssContentLen.str();
    
    envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
    envMap["SERVER_PROTOCOL"] = "HTTP/1.0";
    envMap["REDIRECT_STATUS"] = "200";
    envMap["SERVER_NAME"] = serverConfig->getHost();
    
    std::ostringstream ssPort;
    ssPort << serverConfig->getPort();
    envMap["SERVER_PORT"] = ssPort.str();
    
    envMap["SERVER_SOFTWARE"] = "WebServer/1.0";
    
    std::map<std::string, std::string> headers = req.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        
        if (it->first == "Content-Type") {
            envMap["CONTENT_TYPE"] = it->second;
            continue;
        }
        if (it->first == "Content-Length") continue;
        
        std::string httpKey = "HTTP_";
        for (size_t i = 0; i < it->first.length(); ++i) {
            char c = it->first[i];
            if (c == '-') httpKey += '_';
            else if (c >= 'a' && c <= 'z') httpKey += (char)(c - 'a' + 'A');
            else httpKey += c;
        }
        envMap[httpKey] = it->second;
    }
    
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

Response* CGIhandler::parseCgiOutput(const std::string &output, int /*exitCode*/) {
    
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
        res->setStatus(200);
        res->setBody(output);
        res->addHeader("Content-Type", "text/html");
    }
    
    if (res->getStatus() == 0) {
        res->setStatus(200);
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
                }
                statusSet = true;
            }
        } else {
            res->addHeader(headerName, headerValue);
        }
    }
    
    if (!statusSet) {
        res->setStatus(200);
    }
}