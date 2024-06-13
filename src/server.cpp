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
#include <fstream>
#include <filesystem>
#include <zlib.h>
#include <iomanip>

#define PORT 4221
#define BUFFER_SIZE 1024
#define CONNECTION_BACKLOG 10
std::string directory;

std::mutex cout_mutex; // Mutex for synchronized console output

std::string toHex(const std::string& str) {
    std::ostringstream oss;
    for (unsigned char c : str) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

// Function to compress a string using zlib
std::string gzipCompress(const std::string& str) {
    // Initialize the zlib stream
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("deflateInit2 failed");
    }

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // Process the input string in chunks
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        throw std::runtime_error("Exception during zlib compression: " + std::to_string(ret));
    }

    return outstring;
}

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

    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_size = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_size < 0) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Failed to read data..." << std::endl;
        close(client_fd);
        return;
    }

    std::string request(buffer);
    std::string::size_type pos = request.find(' ');
    if (pos == std::string::npos) {
        close(client_fd);
        return;
    }

    std::string method = request.substr(0, pos);
    std::string::size_type pos2 = request.find(' ', pos + 1);
    if (pos2 == std::string::npos) {
        close(client_fd);
        return;
    }

    std::string path = request.substr(pos + 1, pos2 - pos - 1);
    std::string response;
    bool found = false;
    if (path == "/") {
        response = "HTTP/1.1 200 OK\r\n\r\n";
    } else if (path.find("/echo/") == 0) {
        std::string content = path.substr(6);
        std::string text;
        for(const auto& elem: split(request, "\r\n")){
            if(elem.find("Accept")==0){
                std::vector<std::string> temp = split(elem, " ");
                int n = temp.size();
                for(int i=0; i<n; i++){
                    if(temp[i].find("gzip")==0){
                        found = true;
                        break;
                    }
                }
                break;
            }
        }
        std::string compressed = gzipCompress(content);
        std::string hex = toHex(compressed);

        if(!found){
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
        }
        else{
            response = "HTTP/1.1 200 OK\r\nContent-Encoding: gzip\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(compressed.size()) + "\r\n\r\n" + compressed;
        }
        
    } else if (path.find("/user") == 0) {
        std::string text;
        for (const auto& elem : split(request, "\r\n")) {
            if (elem.find("User") == 0) {
                std::vector<std::string> temp = split(elem, " ");
                if (temp.size() > 1) {
                    text = temp[1];
                }
                break;
            }
        }
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(text.size()) + "\r\n\r\n" + text;
    } else if (path.find("/files/") == 0) {
        std::string filename = path.substr(7);
        std::string filepath = directory + "/" + filename;

        if (method == "GET") {
            if (std::filesystem::exists(filepath)) {
                std::ifstream file(filepath, std::ios::binary);
                std::ostringstream content;
                content << file.rdbuf();
                std::string fileContent = content.str();
                response = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: " + std::to_string(fileContent.size()) + "\r\n\r\n" + fileContent;
            } else {
                response = "HTTP/1.1 404 Not Found\r\n\r\n";
            }
        } else if (method == "POST") {
            size_t contentLength = 0;
            for (const auto& header : split(request, "\r\n")) {
                if (header.find("Content-Length:") == 0) {
                    contentLength = std::stoul(header.substr(16));
                    break;
                }
            }

            std::string::size_type body_pos = request.find("\r\n\r\n");
            if (body_pos != std::string::npos) {
                body_pos += 4;
                std::string body = request.substr(body_pos, contentLength);
                std::ofstream outfile(filepath, std::ios::binary);
                if (outfile) {
                    outfile.write(body.c_str(), body.size());
                    outfile.close();
                    response = "HTTP/1.1 201 Created\r\n\r\n";
                } else {
                    response = "HTTP/1.1 404 Not Found\r\n\r\n";
                }
            } else {
                response = "HTTP/1.1 404 Not Found\r\n\r\n";
            }
        } else {
            response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
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

int main(int argc, char* argv[]) {
    if (argc >= 3 && std::string(argv[1]) == "--directory") {
        directory = argv[2];
    }
    std::cout << "Logs from your program will appear here!\n";
    std::cout << "Files from the directory: " << directory << std::endl;

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

        std::thread client_thread(handleClient, client_fd);
        client_thread.detach();
    }

    close(server_fd);
    return 0;
}
