#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../include/webserv.hpp"
// #include "./EventManager.hpp"  // REMOVED to avoid circular dependency
#include "../config/ServerConfig.hpp"
#include "../config/ConfigParser.hpp"

// Forward declarations
class EventManager;
class Client;

class Server {
private:
    std::vector<int> server_fds;
    std::vector<ServerConfig> configs;
    std::vector<Client*> clients;
    bool running;
    static std::map<std::string, std::string> s_envMap; 
    // Remove this line: EventManager& event_mgr;
public:
    Server(const std::vector<ServerConfig>& configs,
       const std::map<std::string, std::string>& env);

    ~Server();
    void initialize(EventManager& event_mgr);
    void run(EventManager& event_mgr);
    void shutdown();
    void acceptConnection(int server_fd, EventManager& event_mgr);

    // getters
    const std::vector<int>& getServerFds() const;
    const std::vector<ServerConfig>& getConfigs() const;
    const std::vector<Client*>& getClients() const;
    static const std::map<std::string, std::string>& getEnv();
    bool isRunning() const;
};

#endif