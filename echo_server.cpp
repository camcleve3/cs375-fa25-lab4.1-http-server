#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    listen(server_fd, 5);
    std::cout << "Echo server listening on port " << port << std::endl;

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    std::cout << "Client connected from " << inet_ntoa(client_addr.sin_addr) << std::endl;

    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    buffer[bytes_read] = '\0';
    std::cout << "Received: " << buffer << std::endl;
    send(client_fd, buffer, bytes_read, 0); // Echo back

    close(client_fd);
    close(server_fd);
    return 0;
}