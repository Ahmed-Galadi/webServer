#pragma once

#include "../../../../include/webserv.hpp"
#include "../HttpMethodHandler.hpp"
#include "../../../config/LocationConfig.hpp"
#include "../../response/Response.hpp"
#include "../../../server/Server.hpp"

class Client;

enum CgiState {
    CGI_WRITING_BODY,
    CGI_READING_OUTPUT,
    CGI_COMPLETE,
    CGI_ERROR,
    CGI_TIMEOUT
};

struct CgiExecution {
    pid_t pid;
    int socketFd;
    Client* client;
    const ServerConfig* serverConfig;
    std::string scriptPath;
    std::string requestBody;
    size_t bodyBytesWritten;
    std::string output;
    time_t startTime;
    CgiState state;
    int scriptExitCode;
    
    CgiExecution() : pid(-1), socketFd(-1), client(NULL), serverConfig(NULL),
                     bodyBytesWritten(0), startTime(0),
                     state(CGI_WRITING_BODY), scriptExitCode(0) {}
};

class CGIhandler : public HttpMethodHandler {
public:
    CGIhandler();
    virtual ~CGIhandler();
    virtual Response* handler(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig);
    
    static bool startCgiExecution(const Request &req, 
                                  const LocationConfig* location,
                                  const ServerConfig* serverConfig,
                                  Client* client,
                                  class EventManager& eventMgr);
    
    static void handleCgiEvent(int fd, uint32_t events, class EventManager& eventMgr);
    static void cleanupCgiExecution(int fd, class EventManager& eventMgr);
    static void checkCgiTimeouts(class EventManager& eventMgr);
    static std::map<int, CgiExecution*> s_cgiExecutions;

private:
    std::vector<char*> buildEnv(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig);
    Response* parseCgiOutput(const std::string &output, int exitCode);
    void parseCgiHeaders(const std::string &headersStr, Response *res);
    
    static bool forkAndExecCgi(const std::string &scriptPath,
                              const std::vector<char*> &env,
                              int socketPair[2],
                              pid_t& outPid);
    
    static void handleCgiWrite(CgiExecution* exec, class EventManager& eventMgr);
    static void handleCgiRead(CgiExecution* exec, class EventManager& eventMgr);
    static void finalizeCgiExecution(CgiExecution* exec, class EventManager& eventMgr);
    

    
    static const int CGI_TIMEOUT_SECONDS = 30;
};