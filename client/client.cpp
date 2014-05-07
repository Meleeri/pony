#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>

#include <string>
#include <list>
#include <algorithm>

#define CLIENT_PORT 8090
#define MAX_BUFFER 2048

static int client_fd;
static int server_fd;
static std::string username;

static char socket_buf[MAX_BUFFER];

static std::list<std::string> client_list;

void init() {
  sockaddr_in client_addr;
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = ;
  client_addr.sin_port = htonl(CLIENT_PORT);
  if ((::client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("Creating socket for client failed!\n");
      exit(-1);
  };
}

void setusr(const std::string& name) {
  ::username = name;
}

void setsrv(const std::string& server_ip, int server_port) {
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htons(server_ip.tointeger());
  server_addr.sin_port = server_port;
  if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0) {
    perror("Connecting to server failed!\n");
    exit(-1);
  }
}

void parse_list(char *buf, int len) {
  char *end = buf+len;
  std::string client_name;
  while (buf != end) {
    int len = *buf;
    for (int i = 0; i < len; ++ i) {
      client_name.push_back(*buf++);
    }
  }
}

void login() {
  int i = 0;
  socket_buf[i++] = 0x00; //login
  for (auto x = username.begin(); x != username.end(); ++ x) {
    socket_buf[i++] = *x;
  }
  send(client_fd, socket_buf, i, 0);
  int len = recv(client_fd, socket_buf, MAX_BUFFER, 0);
  if (len > 0) {
    parse_list(socket_buf, len);
  }
}

void leave() {
  int i = 0;
  socket_buf[i++] = 0x01; //leave
  for (auto x = username.begin(); x != username.end(); ++ x) {
    socket_buf[i++] = *x;
  }
  send(client_fd, socket_buf, i, 0);
}

void sendmsg(const std::string client_to, char *msg) {
  int i = 0;
  socket_buf[i++] = ::username.size();
  for (auto x = ::username.begin(); x != ::username.end(); ++ x) {
    socket_buf[i++] = *x;
  }
  socket_buf[i++] = client_to.size();
  for (auto x = client_to.begin(); x != client_to.end(); ++ x) {
    socket_buf[i++] = *x;
  }
  socket_buf[i] = '\0';
  strcat(socket_buf, msg);
  
  send(server_fd, socket_buf, i+strlen(msg), 0);
}

void listen_from_server() {
  listen(
}