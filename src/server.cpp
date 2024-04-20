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
// Add this line for sockaddr_in
#include<cstdio>
// Include for send function

#define CRLF "\r\n"
#define HTTP_VER "HTTP/1.1"
#define SP " "
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
  
  // std::cout << "Client connected\n";
  // std::cout << "Sending response\n";
  // const char *response = "HTTP/1.1 200 OK\r\n\r\n";
  // int bytes_sent = send(client_fd, response, strlen(response), 0);
  // if (bytes_sent < 0) {
  //   std::cerr << "Failed to send response\n";
  // } 
  // else {
  //   std::cout << "OK Response sent\n";
  // }
  
  // close(client_fd); // Close the client socket, not the server socket
  // // close(client_fd);
  // close(server_fd);

  // return 0;

  int status  = 200;
  char buff[1024];
  int readbyt;
  readbyt = read(client_fd, buff, 1023);
  buff[readbyt] = '\0';

  do{
    readbyt = read(client_fd, buff, 1023);
    buff[readbyt] = '\0';
    std::cout <<readbyt<<std::endl;
  }while(readbyt>0 and readbyt>=1023);

  char *tok = strtok(buff, " ");
  tok = strtok(NULL, " ");
  std:: cout<<tok ;
  if(strcmp(tok, "/")) status = 400;

  // handling the response
  char *status1 = "200";
  char *reason = "OK";
  std::string res_content(HTTP_VER); 
  res_content.append(SP);
  res_content.append(status1);
  res_content.append(SP);
  res_content.append(reason);
  res_content.append(CRLF);
  res_content.append(CRLF);

  write(client_fd, res_content.c_str(), res_content.length());
 if(status == 200){
  std::string res_content(HTTP_VER); 
  res_content.append(SP);
  res_content.append("200");
  res_content.append(SP);
  res_content.append("OK");
  res_content.append(CRLF);
  res_content.append(CRLF);
  write(client_fd, res_content.c_str(), res_content.length());
 } 
 else {
  std::string res_content(HTTP_VER); 
  res_content.append(SP);
  res_content.append("400");
  res_content.append(SP);
  res_content.append("NOT Found response");
  res_content.append(CRLF);
  res_content.append(CRLF);
  write(client_fd, res_content.c_str(), res_content.length());

 }
 close(client_fd);
 close(server_fd);

 return 0;
}