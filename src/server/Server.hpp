#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../include/webserv.hpp"
#include "../config/ServerConfig.hpp"
#include "../config/ConfigParser.hpp"



class Server {
private:
    std::vector<int> server_fds;
    std::vector<ServerConfig> configs;
    std::vector<Client*> clients;
    bool running;
    static std::map<std::string, std::string> s_envMap; 
public:
    Server(const std::vector<ServerConfig>& configs,
       const std::map<std::string, std::string>& env);

    ~Server();
    void initialize(EventManager& event_mgr);
    void run(EventManager& event_mgr);
    void shutdown();
    void acceptConnection(int server_fd, EventManager& event_mgr);

    const std::vector<int>& getServerFds() const;
    static const std::map<std::string, std::string>& getEnv();
};

#endif