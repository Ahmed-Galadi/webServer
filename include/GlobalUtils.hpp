#ifndef GLOBAL_UTILS_HPP
#define GLOBAL_UTILS_HPP

#include "webserv.hpp"
#include <fcntl.h>

// Set a file descriptor to non-blocking mode. Returns true on success.
bool setToNonBlocking(int fd);

// Common small helpers
std::string numberToString(size_t number);
std::string toLowerCase(const std::string& str);

#endif // GLOBAL_UTILS_HPP
