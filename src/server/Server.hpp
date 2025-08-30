#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../include/webserv.hpp"
#include "./EventManager.hpp"
#include "../config/ServerConfig.hpp"
#include "../config/ConfigParser.hpp"

class Server {
private:
    std::vector<int> server_fds;
    std::vector<ServerConfig> configs;
    std::vector<Client*> clients;
    bool running;
    // Remove this line: EventManager& event_mgr;
public:
    Server(const std::vector<ServerConfig>& configs);
    ~Server();
    void initialize(EventManager& event_mgr);
    void run(EventManager& event_mgr);
    void shutdown();
    void acceptConnection(int server_fd, EventManager& event_mgr);

    // getters
    const std::vector<int>& getServerFds() const;
    const std::vector<ServerConfig>& getConfigs() const;
    const std::vector<Client*>& getClients() const;
    bool isRunning() const;
};

#endif