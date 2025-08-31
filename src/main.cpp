#include "../include/webserv.hpp"
#include "./server/Server.hpp"
#include "./server/EventManager.hpp"



int main(int argc, char* argv[]) {
    try {
        std::string config_file = "default.conf";
        if (argc > 1) {
            config_file = argv[1];
        }
        std::cout << "Starting WebServ...\nConfiguration file: " << config_file << std::endl;

        ConfigParser WSconfig;
        WSconfig.parse(config_file);
        std::vector<ServerConfig> configs = WSconfig.getServers();

        std::cout << "Parsed " << configs.size() << " server blocks" << std::endl;
        for (size_t i = 0; i < configs.size(); ++i) {
            std::cout << "Server " << i + 1 << ": port " << configs[i].getPort()
                      << ", host " << configs[i].getHost()
                      << ", locations " << configs[i].getLocations().size() << std::endl;
        }

        EventManager event_mgr(100);
        Server server(configs);
        server.initialize(event_mgr);
        server.run(event_mgr);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
