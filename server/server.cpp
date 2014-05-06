#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdio>

#include <map>
#include <algorithm>
#include <list>
#include <string>
#include <boost/concept_check.hpp>

#include <boost/thread.hpp>

#define SERVER_PORT 8089
#define MAX_BUFFER 1024

std::map<std::string, int> client_list;

char socket_buf[MAX_BUFFER];

void add_client(const std::string& str, const int ip) {
//  client_list.push_back(str);
  sockaddr_in client_addr;
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = 
}

void remove_client(const std::string& str) {
  /*
  std::remove_if(client_list.begin(), client_list.end(), [&](const std::string& name) {
    return name == str;
  });
  */
}

void transfer_msg(const std::string& client_from, const std::string& client_to, char *msg){
}

void send_client_list(int client_fd) {
}

void listen_client(int client_fd) {
    int len;
    while ((len = recv(client_fd, socket_buf, MAX_BUFFER, 0)) > 0) {
      if (len < 4) {
      }
    }
}

int main() {
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(SERVER_PORT);
  
  int server_fd;
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Failed to create a socket for server ip.\n");
  }
  if ((bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0) {
    perror("Failed to bind server socket to server port.\n");
  };
  
  listen(server_fd, 64);
  
  socklen_t sin_size = sizeof(struct sockaddr);
  while (true) {
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &sin_size);
    if (client_fd < 0) {
	perror("Accepting from client failed.\n");
	continue;
    }
    else {
        boost::thread([&]() {
	  listen_client(client_fd);
	});
    }
  }
}