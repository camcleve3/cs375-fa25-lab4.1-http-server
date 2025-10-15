#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <host> <port> <message>" << std::endl;
        return 1;
    }
    std::string host = argv[1];
    int port = atoi(argv[2]);
    std::string message = argv[3];

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) { perror("socket"); return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (connect(client_fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("connect"); return 1; }

    send(client_fd, message.c_str(), message.length(), 0);
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    buffer[bytes_read] = '\0';
    std::cout << "Echo: " << buffer << std::endl;

    close(client_fd);
    return 0;
}