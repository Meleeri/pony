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

#define SERVER_PORT 64321
#define MAX_BUFFER 1024

struct client_info {
    int fd;
    client_info (int _fd): fd(_fd){};
};

std::map<std::string, client_info> client_list;

void add_client(const std::string& str, int client_fd) {
   char buf[1024];
   /*
    * tell all clients that a new user has logged in
    */
   
   int i = 0;
   buf[i++] = 0x00; //login
   buf[i++] = str.size();
   for (auto x = str.begin(); x != str.end(); ++ x) {
     buf[i++] = *x;
   }
   
   for (auto x = client_list.begin(); x != client_list.end(); ++ x) {
     while (send((x->second).fd, buf, i, 0) != i);
   }
   
   /*
    * add new client
    */
   client_list.insert(std::make_pair(str, client_info(client_fd)));
   
   
   /*
    * notify the newly connected client the client_list
    */

   char *p = buf;
   
   *p++ = 0x03;
   for (auto x = client_list.begin(); x != client_list.end(); ++ x) {
    int len = x->first.size();
    *p++ = len;
    for (int i = 0; i < len; ++ i) {
	*p++ = x->first[i];
    }
   }
   while (send(client_fd, buf, p-buf, 0) != p-buf); 
}

void invalid_login(int client_fd) {
  char buf[16];
  int i = 0;
  buf[i++] = 0x04;
  buf[i++] = 0;
  buf[i++] = 0;
  send(client_fd, buf, i, 0);
}

void remove_client(const std::string& str) {
  /*
   * remove client
   */
   client_list.erase(str);
   
   /*
    * tell all other clients that this client has left
    */
   char buf[1024];
   int i = 0;
   buf[i++] = 0x01; //leave
   buf[i++] = str.size();
   
   for (auto x = str.begin(); x != str.end(); ++ x) {
     buf[i++] = *x;
   }
   
   for (auto x = client_list.begin(); x != client_list.end(); ++ x) {
     while(send((x->second).fd, buf, i, 0) != i);
   }
}

void print_list() {
  printf("Client number: %d\n", client_list.size());
  
  for (auto x = client_list.begin(); x != client_list.end(); ++ x) {
    printf("%s\n", x->first.c_str());
  }
}

void listen_client(int client_fd) {
    char socket_buf[2048];
    
    int len;
    char username[64];
    
    while ((len = recv(client_fd, socket_buf, MAX_BUFFER, 0)) > 0) {
      int code = socket_buf[0];
      int username_len = 0;
      
      username_len = socket_buf[1];
      strncpy(username, socket_buf+2, username_len);
      username[username_len] = '\0';
      
      switch(code) {
	case 0x00: // login
	{
	  auto x = client_list.find(username);
	  if (x != client_list.end()) {
	    invalid_login(client_fd);
	  }
	  else {
	    add_client(username, client_fd);
	    printf("user %s has logged in.\n", username);
	    print_list();
	  }
	}
	  break;
	case 0x01: // leave
	{
	  remove_client(username);
	  
	  printf("user %s has left.\n", username);
	}
	  break;
	case 0x02: // send message 
	  {
	    char client_to[64];
	    int client_to_len = socket_buf[2+username_len];
	    strncpy(client_to, socket_buf+2+username_len+1, client_to_len);
	    
	    client_to[client_to_len] = '\0';
	    int client_to_fd = client_list.find(client_to)->second.fd;
	    while (send(client_to_fd, socket_buf, len, 0) < 0);    
	    
	    printf("user %s has sent a message\n", username);
	  }
	  break;
      }
    }
    close(client_fd);
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
	printf("Spawning new thread.\n");
        boost::thread([&]() {
	  listen_client(client_fd);
	});
    }
  }
  close(server_fd);
  return 0;
}