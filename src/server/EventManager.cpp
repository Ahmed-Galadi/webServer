#include "EventManager.hpp"
#include "Server.hpp"
#include "../client/Client.hpp"
#include "../../include/webserv.hpp"

EventManager::EventManager(int max_events) : epoll_fd(-1), events(NULL), max_events(max_events)
{
    epoll_fd = epoll_create(max_events);
    if (epoll_fd == -1) {
        throw std::runtime_error("Failed to create epoll file descriptor");
    }
    events = new epoll_event[max_events];
    std::memset(events, 0, sizeof(epoll_event) * max_events);
}

EventManager::~EventManager()
{
    if (events)
    {
        delete[] events;
    }
    if (epoll_fd != -1)
    {
        close(epoll_fd);
    }
}

void EventManager::addSocket(int fd, void* data, uint32_t event_flags) {
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = event_flags;
    event.data.ptr = data;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        throw std::runtime_error("Failed to add socket to epoll");
    }
}

void EventManager::removeSocket(int fd) {
    if (fd <= 0) {
        std::cerr << "Attempt to remove invalid FD: " << fd << std::endl;
        return;
    }
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        std::cerr << "Failed to remove FD " << fd << ": " << strerror(errno) << std::endl;
    }
}

void EventManager::modifySocket(int fd, void* data, uint32_t event_flags)
{
    epoll_event event;
    std::memset(&event, 0, sizeof(event));
    event.events = event_flags;
    event.data.ptr = data;
    
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
        std::cerr << "[ERROR] Failed to modify socket " << fd << " in epoll: " 
                  << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to modify socket in epoll");
    }
    
}

int EventManager::waitForEvents(epoll_event* event_buffer, int timeout)
{
    int result = epoll_wait(epoll_fd, event_buffer, max_events, timeout);
    
    return result;
}