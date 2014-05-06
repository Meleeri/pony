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

struct client_info {
    int fd;
    client_info (int _fd): fd(_fd){};
};

std::map<std::string, client_info> client_list;

void add_client(const std::string& str, const int client_fd) {
   client_list.insert(std::make_pair(str, client_info(client_fd)));
}

void remove_client(const std::string& str) {
   client_list.erase(str);
}

void transfer_msg(const std::string& client_from, const std::string& client_to, char *msg, int len){
  int client_to_fd = client_list.find(client_to)->second.fd;
  send(client_to_fd, msg, len, 0);
}

void send_client_list(int client_fd) {
  char *buf = (char*)malloc(sizeof(char)*1024);
  char *p = buf;
  for (auto x = client_list.begin(); x != client_list.end(); ++ x) {
    int len = x->first.size();
    *p++ = len;
    for (int i = 0; i < len; ++ i) {
	*p++ = x->first[i];
    }
  }
  send(client_fd, buf, p-buf, 0);
}

void listen_client(int client_fd) {
    char *socket_buf = (char*)malloc(MAX_BUFFER);
    int len;
    char username[64];
    
    while ((len = recv(client_fd, socket_buf, MAX_BUFFER, 0)) > 0) {
      int code = socket_buf[0];
      int username_len = 0;
      
      username_len = socket_buf[1];
      strncpy(username, socket_buf+2, username_len);
      
      switch(code) {
	case 0x00: // connect
	  add_client(username, client_fd);
	  send_client_list(client_fd);
	  break;
	case 0x01: // leave
	  remove_client(username);
	  break;
	case 0x11: // send message 
	  {
	    char client_to[64];
	    int client_to_len = socket_buf[2+username_len];
	    strncpy(client_to, socket_buf+2+username_len+1, client_to_len);
	    transfer_msg(username, client_to, socket_buf+2+username_len+1+client_to_len, strlen(socket_buf)-3-username_len-client_to_len);
	  }
	  break;
      }
    }
    delete socket_buf;
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