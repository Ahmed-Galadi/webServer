#include "Server.hpp"
#include "./EventManager.hpp"
#include "../client/Client.hpp"
#include "../http/httpMethods/cgi/CGIhandler.hpp"
#include "../../include/GlobalUtils.hpp"
#include "../../include/webserv.hpp"

Server::Server(const std::vector<ServerConfig>& configs,
       const std::map<std::string, std::string>& env) : configs(configs), running(false) {
    s_envMap = env;
}

std::map<std::string, std::string> Server::s_envMap;

Server::~Server() {
	shutdown();
}

const std::map<std::string, std::string>& Server::getEnv() {
    return s_envMap;
}


void Server::initialize(EventManager& event_manager) {
	for (size_t i = 0; i < configs.size(); ++i) {
		int server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd == -1) {
			std::cerr << "[ERROR] Failed to create socket: " << strerror(errno) << std::endl;
			throw std::runtime_error("Failed to create socket");
		}

		if (!setToNonBlocking(server_fd)) {
			std::cerr << "[ERROR] Failed to set socket to non-blocking: " << strerror(errno) << std::endl;
			close(server_fd);
			throw std::runtime_error("Failed to set socket to non-blocking");
		}

		int opt = 1;
		if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
			std::cerr << "[ERROR] Failed to set SO_REUSEADDR: " << strerror(errno) << std::endl;
			close(server_fd);
			throw std::runtime_error("Failed to set SO_REUSEADDR");
		}

		sockaddr_in server_addr;
		std::memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(configs[i].getPort());
		
		if (configs[i].getHost() == "0.0.0.0" || configs[i].getHost().empty()) {
			server_addr.sin_addr.s_addr = INADDR_ANY;
		} else {
			if (inet_pton(AF_INET, configs[i].getHost().c_str(), &server_addr.sin_addr) <= 0) {
				close(server_fd);
				throw std::runtime_error("Invalid host address: " + configs[i].getHost());
			}
		}
		if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
			std::cerr << "[ERROR] Failed to bind socket to port " << configs[i].getPort() 
					  << ": " << strerror(errno) << std::endl;
			close(server_fd);
			std::ostringstream oss;
			oss << configs[i].getPort();
			throw std::runtime_error("Failed to bind socket to port " + oss.str());
		}

		if (listen(server_fd, SOMAXCONN) == -1) {
			close(server_fd);
			throw std::runtime_error("Failed to listen on socket");
		}

		server_fds.push_back(server_fd);
		try {
			event_manager.addSocket(server_fd, this, EPOLLIN);
		} catch (const std::exception& e) {
			std::cerr << "[ERROR] Failed to register server socket in epoll: " << e.what() << std::endl;
			close(server_fd);
			server_fds.pop_back();
			throw;
		}
		
		std::cout << "[INFO] Server listening on " << configs[i].getHost() 
				  << ":" << configs[i].getPort() << " with fd=" << server_fd << std::endl;
	}
}

void Server::run(EventManager& event_manager) {
	running = true;
	std::cout << "[INFO] Server started, waiting for connections..." << std::endl;
	
	epoll_event* events = new epoll_event[100];
	
	while (running) {
        CGIhandler::checkCgiTimeouts(event_manager);

		int nfds = event_manager.waitForEvents(events, 1000);
		
		if (nfds == -1) {
			continue;
		}
		
for (int i = 0; i < nfds; ++i)
{
    epoll_event& event = events[i];

    if (event.data.ptr == this) 
    {
        for (size_t j = 0; j < server_fds.size(); ++j) {
            if (event.events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
                acceptConnection(server_fds[j], event_manager);
            }
        }
    }
    else
    {
        bool isCgiEvent = false;
        
        for (std::map<int, CgiExecution*>::iterator it = CGIhandler::s_cgiExecutions.begin();
             it != CGIhandler::s_cgiExecutions.end(); ++it) {
            if (it->second == event.data.ptr) {
                CGIhandler::handleCgiEvent(it->first, event.events, event_manager);
                isCgiEvent = true;
                break;
            }
        }
        
        if (!isCgiEvent) {
            Client* client = static_cast<Client*>(event.data.ptr);
            
            if (!client) {
                std::cerr << "[ERROR] Null client pointer in event" << std::endl;
                continue;
            }
            
            bool client_valid = false;
            for (size_t j = 0; j < clients.size(); ++j) {
                if (clients[j] == client) {
                    client_valid = true;
                    break;
                }
            }
            
            if (!client_valid) {
                continue;
            }
            
            if (event.events & (EPOLLHUP | EPOLLERR))
            {
                client->closeConnection(event_manager);
                clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
                delete client;
            }
            else if (event.events & EPOLLIN)
            {
                client->handleRead(event_manager);
            }
            else if (event.events & EPOLLOUT)
            {
                if (client->isWaitingForCgi()) {
                    continue;
                }
                client->handleWrite(event_manager);
            }
        }
    }
}
	}
	
	delete[] events;
}

void Server::shutdown() {
	running = false;
	
	for (size_t i = 0; i < clients.size(); ++i) {
		delete clients[i];
	}
	clients.clear();
	
	for (size_t i = 0; i < server_fds.size(); ++i) {
		close(server_fds[i]);
	}
	server_fds.clear();
}

void Server::acceptConnection(int server_fd, EventManager& event_manager)
{
	sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	
	int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
	if (client_fd == -1) {
		return;
	}
	
	if (!setToNonBlocking(client_fd)) {
		std::cerr << "[ERROR] Failed to set client socket to non-blocking: " << strerror(errno) << std::endl;
		close(client_fd);
		return;
	}
	
	ServerConfig* config = NULL;
	for (size_t i = 0; i < server_fds.size(); ++i) {
		if (server_fds[i] == server_fd) {
			config = &configs[i];
			break;
		}
	}
	
	if (!config) {
		std::cerr << "[ERROR] Could not find server config for accepted connection" << std::endl;
		close(client_fd);
		return;
	}
	
	Client* client = new Client(client_fd, config);
	client->setEventManager(&event_manager);
	clients.push_back(client);

	try {
		event_manager.addSocket(client_fd, client, EPOLLIN | EPOLLERR | EPOLLHUP);
	} catch (const std::exception& e) {
		std::cerr << "[ERROR] Failed to register client socket in epoll: " << e.what() << std::endl;
		clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
		delete client;
		close(client_fd);
		return;
	}

	
	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
	std::cout << "[INFO] New connection from " << client_ip 
			  << ":" << ntohs(client_addr.sin_port) << " on fd=" << client_fd << std::endl;
}
const std::vector<int>& Server::getServerFds() const {
	return server_fds;
}