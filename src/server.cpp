#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h> 
#include <bits/stdc++.h>
// Add this line for sockaddr_in
#include<cstdio>
// Include for send function
using namespace std;
#define CRLF "\r\n"
#define HTTP_VER "HTTP/1.1" 
#define SP " "
vector<string> split(string &request, string delim)
{
    stringstream ss (request);
    vector<string> comps;
    string temp;
    while(getline(ss, temp, delim[0]))
    {
      comps.push_back(temp);
      ss.ignore(delim.size()-1);
    }
    return comps;
}
int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";
  // descirptor
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);  
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }
  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
 
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr); // Change int to socklen_t
  // socklen_t client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
  if (client_fd < 0) {
    std::cerr << "Failed to accept client connection\n";
    return 1;
  }




  
char http_req[BUFSIZ];
ssize_t bytes_size = read(client_fd, http_req, BUFSIZ);
if(bytes_size<0){
  cerr<<"failed to read data...";
}
http_req[bytes_size]= '\0';
string path;
string request(http_req);

vector<string> comps = split(request, "\r\n");

size_t methodEnd = request.find(' ');
if(methodEnd != std::string::npos){
  auto start = methodEnd + 1;
  auto end = request.find(' ', start);

  if(end!=std::string::npos){
    path = request.substr(start, end-start);
  }
}

string response;
if(path =="/"){
  response = "HTTP/1.1 200 OK\r\n\r\n";
}
else if(path.find("/echo/") == 0){
  std::string content = path.substr(6);
  response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "+ to_string(content.size()) + "\r\n\r\n" + content;
}
else if(path.find("/user")==0){
  string text;
  for(auto &elem:comps){
    if(elem.find("User")==0){
      vector<string> temp = split(elem, " ");
      text =temp[1];
      break;
    }
  }
  response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:" + to_string(text.size()) + "\r\n\r\n" + text;
}
else{
   response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
}
ssize_t server_send =send(client_fd, response.c_str(), response.size(), 0);

if(server_send==-1){
  std:: cerr<<"Server failed to send response"<< std:: endl;
  close(client_fd);
  return -1;
}
close(client_fd);
close(server_fd);
return 0;
}
