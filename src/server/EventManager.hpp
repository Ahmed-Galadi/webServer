// ============ src/server/ConnectionManager.hpp ============
#ifndef CONNECTIONMANAGER_HPP
#define CONNECTIONMANAGER_HPP

#include "../../include/webserv.hpp"

#ifndef EVENT_MANAGER_HPP
#define EVENT_MANAGER_HPP

class Server;
class Client;

class EventManager {
private:
    int epoll_fd;
    epoll_event* events;
    int max_events;
public:
    EventManager(int max_events = 100);
    ~EventManager();
    void addSocket(int fd, void* data, uint32_t event_flags);
    void removeSocket(int fd);
    void modifySocket(int fd, void* data, uint32_t events);
    int waitForEvents(epoll_event* event_buffer, int timeout);
};

#endif

#endif
