#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

std::queue<int> task_queue;
std::mutex queue_mutex;
std::condition_variable cv;
bool stop_pool = false;
const int NUM_THREADS = 4;

void send_response(int client_fd, const std::string& content, const std::string& content_type, int status = 200) {
    std::ostringstream oss;
    if (status == 200) {
        oss << "HTTP/1.1 200 OK\r\n";
    } else {
        oss << "HTTP/1.1 404 Not Found\r\n";
    }
    oss << "Content-Type: " << content_type << "\r\n"
        << "Content-Length: " << content.length() << "\r\n"
        << "Connection: close\r\n\r\n"
        << content;
    std::string response = oss.str();
    write(client_fd, response.c_str(), response.length());
}

void worker() {
    while (true) {
        int client_fd;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, []{ return !task_queue.empty() || stop_pool; });
            if (stop_pool && task_queue.empty()) return;
            client_fd = task_queue.front(); task_queue.pop();
        }
        char buffer[4096];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            close(client_fd);
            continue;
        }
        buffer[bytes_read] = '\0';
        std::istringstream req(buffer);
        std::string method, path;
        req >> method >> path;
        if (method == "GET") {
            if (path == "/") path = "/index.html";
            std::string file_path = "www" + path;
            std::ifstream file(file_path, std::ios::binary);
            if (file) {
                std::ostringstream content;
                content << file.rdbuf();
                std::string content_type = "text/html";
                if (path.find(".css") != std::string::npos) content_type = "text/css";
                if (path.find(".png") != std::string::npos) content_type = "image/png";
                send_response(client_fd, content.str(), content_type, 200);
            } else {
                send_response(client_fd, "<h1>404 Not Found</h1>", "text/html", 404);
            }
        } else {
            send_response(client_fd, "<h1>400 Bad Request</h1>", "text/html", 400);
        }
        close(client_fd);
    }
}

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
    std::cout << "HTTP server listening on port " << port << std::endl;

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i)
        threads.emplace_back(worker);

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) continue;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            task_queue.push(client_fd);
        }
        cv.notify_one();
    }
    stop_pool = true;
    cv.notify_all();
    for (auto& t : threads) t.join();
    close(server_fd);
    return 0;
}