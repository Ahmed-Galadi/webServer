#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>

#define PORT 8080
#define MAX_CLIENTS 10

int set_non_blocking(int fd) {
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    // Allow address reuse
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    set_non_blocking(server_fd);
    listen(server_fd, SOMAXCONN);

    std::cout << "Server listening on port " << PORT << "\n";

    struct pollfd fds[MAX_CLIENTS];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    int nfds = 1;

    while (true) {
        int activity = poll(fds, nfds, -1);
        if (activity < 0) {
            std::cerr << "Poll error\n";
            break;
        }

        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
			memset(&client_addr, 0, sizeof(client_addr));
            socklen_t len = sizeof(client_addr);
            int client_fd = accept(server_fd, (sockaddr *)&client_addr, &len);
            if (client_fd >= 0) {
                set_non_blocking(client_fd);
                fds[nfds].fd = client_fd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        for (int i = 1; i < nfds; ++i) {
            if (fds[i].revents & POLLIN) {
                char buffer[1024] = {0};
                int valread = read(fds[i].fd, buffer, sizeof(buffer));
                if (valread > 0) {
                    std::cout << "Received:\n" << buffer << "\n";

                    const char *response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 25\r\n"
                        "\r\n"
                        "ITS WORKING ! YEAH BITCH!";
                    send(fds[i].fd, response, strlen(response), 0);
                }
                // close(fds[i].fd);
                // fds[i] = fds[nfds - 1];
                // nfds--;
                // i--; // check new entry in this slot
            }
        }
    }

    close(server_fd);
    return 0;
}
