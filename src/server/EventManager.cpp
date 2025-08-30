// ============ src/server/EventManager.cpp ============
#include "EventManager.hpp"
#include "Server.hpp"
#include "../client/Client.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>

EventManager::EventManager(int max_events) : epoll_fd(-1), events(NULL), max_events(max_events)
{
    epoll_fd = epoll_create1(0);
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
    event.data.ptr = data;  // Only set the pointer - don't set fd!
    
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
    event.data.ptr = data;  // Only set the pointer
    
    std::cout << "[DEBUG] Modifying socket fd=" << fd << " with events=" << event_flags << std::endl;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
        std::cerr << "[ERROR] Failed to modify socket " << fd << " in epoll: " 
                  << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to modify socket in epoll");
    }
    
    std::cout << "[DEBUG] Successfully modified socket " << fd << " in epoll" << std::endl;
}

int EventManager::waitForEvents(epoll_event* event_buffer, int timeout)
{
    std::cout << "[DEBUG] Calling epoll_wait with timeout=" << timeout << std::endl;
    int result = epoll_wait(epoll_fd, event_buffer, max_events, timeout);
    std::cout << "[DEBUG] epoll_wait returned: " << result << std::endl;
    
    if (result > 0) {
        for (int i = 0; i < result; ++i) {
            std::cout << "[DEBUG] Event " << i << ": events=" << event_buffer[i].events 
                      << ", ptr=" << event_buffer[i].data.ptr << std::endl;
        }
    }
    
    return result;
}

int EventManager::getEpollFd() const {
    return epoll_fd;
}