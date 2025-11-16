#include "../../include/GlobalUtils.hpp"
#include <unistd.h>
#include <errno.h>

bool setToNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return false;
    return true;
}

std::string numberToString(size_t number) {
    std::ostringstream oss;
    oss << number;
    return oss.str();
}

std::string toLowerCase(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i) {
        result[i] = std::tolower(static_cast<unsigned char>(result[i]));
    }
    return result;
}
