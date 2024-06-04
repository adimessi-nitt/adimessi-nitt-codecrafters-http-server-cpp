
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>

#define PORT 4221
#define BUFFER_SIZE 1024
#define CONNECTION_BACKLOG 5

std::mutex cout_mutex; // Mutex for synchronized console output

std::vector<std::string> split(const std::string& request, const std::string& delim) {
    std::stringstream ss(request);
    std::vector<std::string> comps;
    std::string temp;
    while (std::getline(ss, temp, delim[0])) {
        comps.push_back(temp);
        ss.ignore(delim.size() - 1);
    }
    return comps;
}

void handleClient(int client_fd) {
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Handling new client on thread: " << std::this_thread::get_id() << std::endl;
    }

    char http_req[BUFFER_SIZE];
    ssize_t bytes_size = read(client_fd, http_req, BUFFER_SIZE);
    if (bytes_size < 0) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Failed to read data..." << std::endl;
        close(client_fd);
        return;
    }

    http_req[bytes_size] = '\0';
    std::string request(http_req);
    std::string path;
    std::vector<std::string> comps = split(request, "\r\n");

    size_t methodEnd = request.find(' ');
    if (methodEnd != std::string::npos) {
        auto start = methodEnd + 1;
        auto end = request.find(' ', start);

        if (end != std::string::npos) {
            path = request.substr(start, end - start);
        }
    }

    std::string response;
    if (path == "/") {
        response = "HTTP/1.1 200 OK\r\n\r\n";
    } else if (path.find("/echo/") == 0) {
        std::string content = path.substr(6);
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
    } else if (path.find("/user") == 0) {
        std::string text;
        for (const auto& elem : comps) {
            if (elem.find("User") == 0) {
                std::vector<std::string> temp = split(elem, " ");
                if (temp.size() > 1) {
                    text = temp[1];
                }
                break;
            }
        }
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(text.size()) + "\r\n\r\n" + text;
    } else {
        response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }

    ssize_t server_send = send(client_fd, response.c_str(), response.size(), 0);
    if (server_send == -1) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Server failed to send response" << std::endl;
    }

    close(client_fd);
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Client handled successfully on thread: " << std::this_thread::get_id() << std::endl;
    }
}

int main() {
    std::cout << "Logs from your program will appear here!\n";

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        close(server_fd);
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port " << PORT << "\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, CONNECTION_BACKLOG) != 0) {
        std::cerr << "listen failed\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "\n";

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            std::cerr << "Failed to accept client connection\n";
            continue;
        }

        // Handle each client connection in a separate thread
        std::thread client_thread(handleClient, client_fd);
        client_thread.detach(); // Detach the thread to allow independent execution
    }

    close(server_fd);
    return 0;
}
