#pragma once

#include "../HttpMethodHandler.hpp"
#include "../../../config/LocationConfig.hpp"
#include "../../response/Response.hpp"
#include "../../../server/Server.hpp"
#include <map>
#include <string>
#include <vector>
#include <sys/types.h>

// Forward declaration
class Client;

// Enum for CGI execution state
enum CgiState {
    CGI_WRITING_BODY,    // Writing request body to CGI
    CGI_READING_OUTPUT,  // Reading CGI output
    CGI_COMPLETE,        // CGI finished successfully
    CGI_ERROR,           // CGI error occurred
    CGI_TIMEOUT          // CGI timed out
};

// Structure to track ongoing CGI execution
struct CgiExecution {
    pid_t pid;                    // Child process ID
    int socketFd;                 // Parent's end of socketpair
    Client* client;               // Associated client
    std::string scriptPath;       // Path to CGI script
    std::string requestBody;      // Request body to send
    size_t bodyBytesWritten;      // Bytes written so far
    std::string output;           // Accumulated output from CGI
    time_t startTime;             // When CGI started
    CgiState state;               // Current state
    int scriptExitCode;           // Exit code from script
    
    CgiExecution() : pid(-1), socketFd(-1), client(NULL), 
                     bodyBytesWritten(0), startTime(0),
                     state(CGI_WRITING_BODY), scriptExitCode(0) {}
};

class CGIhandler : public HttpMethodHandler {
public:
    CGIhandler();
    virtual ~CGIhandler();
    virtual Response* handler(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig);
    
    // New: Async CGI methods
    static bool startCgiExecution(const Request &req, 
                                  const LocationConfig* location,
                                  const ServerConfig* serverConfig,
                                  Client* client,
                                  class EventManager& eventMgr);
    
    static void handleCgiEvent(int fd, uint32_t events, class EventManager& eventMgr);
    static void cleanupCgiExecution(int fd, class EventManager& eventMgr);
    static void checkCgiTimeouts(class EventManager& eventMgr);
        // Static map to track all ongoing CGI executions
    // Key: socket FD, Value: execution state
    static std::map<int, CgiExecution*> s_cgiExecutions;

private:
    std::vector<char*> buildEnv(const Request &req, const LocationConfig* location, const ServerConfig* serverConfig);
    Response* parseCgiOutput(const std::string &output, int exitCode);
    void parseCgiHeaders(const std::string &headersStr, Response *res);
    
    // New: Helper methods for async execution
    static bool forkAndExecCgi(const std::string &scriptPath,
                              const std::vector<char*> &env,
                              int socketPair[2],
                              pid_t& outPid);
    
    static void handleCgiWrite(CgiExecution* exec, class EventManager& eventMgr);
    static void handleCgiRead(CgiExecution* exec, class EventManager& eventMgr);
    static void finalizeCgiExecution(CgiExecution* exec, class EventManager& eventMgr);
    

    
    // CGI timeout in seconds
    static const int CGI_TIMEOUT = 30;
};