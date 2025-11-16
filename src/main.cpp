#include "../include/webserv.hpp"
#include "../src/config/ConfigParser.hpp"
#include "../src/server/Server.hpp"
#include "../src/server/EventManager.hpp"

int main(int argc, char* argv[], char* envp[]) {
    try {
        // Default config file path
        std::string config_file = "config/default2.conf";
        
        if (argc > 1) {
            config_file = argv[1];
        }
        
        std::cout << "Starting WebServ...\nConfiguration file: " << config_file << std::endl;

        // 1️⃣ Parse the config file
        ConfigParser WSconfig;
        WSconfig.parse(config_file);
        std::vector<ServerConfig> configs = WSconfig.getServers();

        std::cout << "Parsed " << configs.size() << " server blocks" << std::endl;
        for (size_t i = 0; i < configs.size(); ++i) {
            std::cout << "Server " << i + 1 << ": port " << configs[i].getPort()
                      << ", host " << configs[i].getHost()
                      << ", locations " << configs[i].getLocations().size() << std::endl;
        }

        // 2️⃣ Build the environment map from envp
        std::map<std::string, std::string> envMap;
        for (int i = 0; envp[i]; ++i) {
            std::string entry(envp[i]);
            size_t pos = entry.find('=');
            if (pos != std::string::npos) {
                envMap[entry.substr(0, pos)] = entry.substr(pos + 1);
            }
        }

        // 3️⃣ Create the EventManager and Server
        EventManager event_mgr(100);
        Server server(configs, envMap);  // ✅ pass env

        // 4️⃣ Start server
        server.initialize(event_mgr);
        server.run(event_mgr);
    } 
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
