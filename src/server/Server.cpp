// ============ src/server/Server.cpp ============
#include "Server.hpp"
#include "./EventManager.hpp"
#include "../client/Client.hpp"
#include "../http/httpMethods/cgi/CGIhandler.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <algorithm>

Server::Server(const std::vector<ServerConfig>& configs,
       const std::map<std::string, std::string>& env) : configs(configs), running(false) {
    s_envMap = env;  // ✅ Set once in constructor
    // ... rest of initialization
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
	//	std::cout << "[DEBUG] Initializing server " << i + 1 << " on " 
		//		  << configs[i].getHost() << ":" << configs[i].getPort() << std::endl;
		
		int server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd == -1) {
			std::cerr << "[ERROR] Failed to create socket: " << strerror(errno) << std::endl;
			throw std::runtime_error("Failed to create socket");
		}
	//	std::cout << "[DEBUG] Created socket fd=" << server_fd << std::endl;

		// Set socket to non-blocking
		int flags = fcntl(server_fd, F_GETFL, 0);
		if (flags == -1 || fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
			std::cerr << "[ERROR] Failed to set socket to non-blocking: " << strerror(errno) << std::endl;
			close(server_fd);
			throw std::runtime_error("Failed to set socket to non-blocking");
		}
	//	std::cout << "[DEBUG] Set socket fd=" << server_fd << " to non-blocking" << std::endl;

		// Set SO_REUSEADDR
		int opt = 1;
		if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
			std::cerr << "[ERROR] Failed to set SO_REUSEADDR: " << strerror(errno) << std::endl;
			close(server_fd);
			throw std::runtime_error("Failed to set SO_REUSEADDR");
		}
	//	std::cout << "[DEBUG] Set SO_REUSEADDR on socket fd=" << server_fd << std::endl;

		// Bind socket
		sockaddr_in server_addr;
		std::memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(configs[i].getPort());
		
		if (configs[i].getHost() == "0.0.0.0" || configs[i].getHost().empty()) {
			server_addr.sin_addr.s_addr = INADDR_ANY;
		//	std::cout << "[DEBUG] Binding to INADDR_ANY (0.0.0.0)" << std::endl;
		} else {
			if (inet_pton(AF_INET, configs[i].getHost().c_str(), &server_addr.sin_addr) <= 0) {
			//	std::cerr << "[ERROR] Invalid host address: " << configs[i].getHost() << std::endl;
				close(server_fd);
				throw std::runtime_error("Invalid host address: " + configs[i].getHost());
			}
		//	std::cout << "[DEBUG] Binding to host: " << configs[i].getHost() << std::endl;
		}
		std::cout << "------------------------------------- server fd:" << server_fd << std::endl;
		if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
			std::cerr << "[ERROR] Failed to bind socket to port " << configs[i].getPort() 
					  << ": " << strerror(errno) << std::endl;
			close(server_fd);
			std::ostringstream oss;
			oss << configs[i].getPort();
			throw std::runtime_error("Failed to bind socket to port " + oss.str());
		}
	//	std::cout << "[DEBUG] Bound socket fd=" << server_fd << " to port " << configs[i].getPort() << std::endl;

		// Start listening
		if (listen(server_fd, SOMAXCONN) == -1) {
		//	std::cerr << "[ERROR] Failed to listen on socket: " << strerror(errno) << std::endl;
			close(server_fd);
			throw std::runtime_error("Failed to listen on socket");
		}
	//	std::cout << "[DEBUG] Socket fd=" << server_fd << " is now listening" << std::endl;

		server_fds.push_back(server_fd);
		event_manager.addSocket(server_fd, this, EPOLLIN);
		
		std::cout << "[INFO] Server listening on " << configs[i].getHost() 
				  << ":" << configs[i].getPort() << " with fd=" << server_fd << std::endl;
	}
}

void Server::run(EventManager& event_manager) {
	running = true;
	std::cout << "[INFO] Server started, waiting for connections..." << std::endl;
	
	epoll_event* events = new epoll_event[100]; // Create local event buffer
	
	while (running) {
		   // Check CGI timeouts before waiting for events
        CGIhandler::checkCgiTimeouts(event_manager);

		int nfds = event_manager.waitForEvents(events, -1); // 1 second timeout
		
		if (nfds == -1) {
			// Per subject: cannot check errno after I/O operations
			// epoll_wait returning -1 could be signal interruption or real error
			// Since we can't check errno, we'll just log and continue
			// A real error will likely repeat and cause other observable issues
			std::cout << "[DEBUG] epoll_wait returned -1, continuing" << std::endl;
			continue;
		}
		
for (int i = 0; i < nfds; ++i)
{
    epoll_event& event = events[i];
    // std::cout << "[DEBUG] Processing event " << i << ": events=" << event.events 
    //           << ", ptr=" << event.data.ptr << std::endl;

    // Check if this is a server socket (new connection)
    if (event.data.ptr == this) 
    {
     //   std::cout << "[DEBUG] Server socket event detected, events=" << event.events << std::endl;
        // Try accepting on all server sockets that have events
        for (size_t j = 0; j < server_fds.size(); ++j) {
            if (event.events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
              //  std::cout << "[DEBUG] Attempting to accept on server_fd=" << server_fds[j] << std::endl;
                acceptConnection(server_fds[j], event_manager);
            }
        }
    }
    else
    {
        // Try to identify if this is a CGI event
        bool isCgiEvent = false;
        
        // Check all CGI executions to see if this event belongs to one
        for (std::map<int, CgiExecution*>::iterator it = CGIhandler::s_cgiExecutions.begin();
             it != CGIhandler::s_cgiExecutions.end(); ++it) {
            if (it->second == event.data.ptr) {
                // This is a CGI event
                std::cout << "[DEBUG] Handling CGI event for FD=" << it->first << std::endl;
                CGIhandler::handleCgiEvent(it->first, event.events, event_manager);
                isCgiEvent = true;
                break;
            }
        }
        
        if (!isCgiEvent) {
            // This is a client socket event
            Client* client = static_cast<Client*>(event.data.ptr);
            
            // Validate the client pointer
            if (!client) {
                std::cerr << "[ERROR] Null client pointer in event" << std::endl;
                continue;
            }
            
            // Additional validation: check if client is in our clients vector
            bool client_valid = false;
            for (size_t j = 0; j < clients.size(); ++j) {
                if (clients[j] == client) {
                    client_valid = true;
                    break;
                }
            }
            
            if (!client_valid) {
             //   std::cerr << "[ERROR] Invalid client pointer in event" << std::endl;
                continue;
            }
            
           // std::cout << "[DEBUG] Client event for fd=" << client->getFd() << std::endl;
            
            if (event.events & (EPOLLHUP | EPOLLERR))
            {
              //  std::cout << "[DEBUG] Client connection closed or error for fd=" << client->getFd() << std::endl;
                client->closeConnection(event_manager);
                clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
                delete client;
            }
            else if (event.events & EPOLLIN)
            {
              //  std::cout << "[DEBUG] Handling read for client fd=" << client->getFd() << std::endl;
                client->handleRead(event_manager);
            }
            else if (event.events & EPOLLOUT)
            {
             //   std::cout << "[DEBUG] Handling write for client fd=" << client->getFd() << std::endl;
                // Check if waiting for CGI
                if (client->isWaitingForCgi()) {
                    std::cout << "[DEBUG] Client waiting for CGI, skipping write" << std::endl;
                    continue;
                }
                client->handleWrite(event_manager);
            }
        }
    }
}
		// Clean up timed out clients
		for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end();) {
			if ((*it)->isTimedOut()) {
			//	std::cout << "[DEBUG] Cleaning up timed out client fd=" << (*it)->getFd() << std::endl;
				(*it)->closeConnection(event_manager);
				delete *it;
				it = clients.erase(it);
			} else {
				++it;
			}
		}
	}
	
	delete[] events;
}

void Server::shutdown() {
	running = false;
	
	// Close all client connections
	for (size_t i = 0; i < clients.size(); ++i) {
		delete clients[i];
	}
	clients.clear();
	
	// Close all server sockets
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
		// Per subject requirements: CANNOT check errno after I/O operations
		// For non-blocking accept(), -1 is normal when no connections are pending
		// Just return and wait for next epoll event
		return;
	}
	
	// Set client socket to non-blocking
	int flags = fcntl(client_fd, F_GETFL, 0);
	if (flags == -1 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		std::cerr << "[ERROR] Failed to set client socket to non-blocking: " << strerror(errno) << std::endl;
		close(client_fd);
		return;
	}
	
	// Find the appropriate server config
	ServerConfig* config = NULL;
	for (size_t i = 0; i < server_fds.size(); ++i) {
		if (server_fds[i] == server_fd) {
			// std::cout << "[DEBUG] Accepting new connection on server_fd=" << server_fd 
			// 		  << ", client_fd=" << client_fd << std::endl;
			config = &configs[i];
			break;
		}
	}
	
	if (!config) {
		std::cerr << "[ERROR] Could not find server config for accepted connection" << std::endl;
		close(client_fd);
		return;
	}
	
    // Create new client
    Client* client = new Client(client_fd, config);
    client->setEventManager(&event_manager);  // ADD THIS LINE
    clients.push_back(client);
	
	// Add client to epoll in Level-Triggered mode (not Edge-Triggered)
	// LT mode: epoll continues to notify while data is available
	// ET mode: epoll only notifies once when new data arrives (would require reading all data at once)
	// Since we read once per epoll event (per subject requirements), we need LT mode
	event_manager.addSocket(client_fd, client, EPOLLIN | EPOLLERR | EPOLLHUP);

	
	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
	std::cout << "[INFO] New connection from " << client_ip 
			  << ":" << ntohs(client_addr.sin_port) << " on fd=" << client_fd << std::endl;
}
// Getters
const std::vector<int>& Server::getServerFds() const {
	return server_fds;
}

const std::vector<ServerConfig>& Server::getConfigs() const {
	return configs;
}

const std::vector<Client*>& Server::getClients() const {
	return clients;
}

bool Server::isRunning() const {
	return running;
}