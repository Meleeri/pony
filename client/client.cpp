#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdio>
#include <cstring>

#include <string>
#include <list>
#include <algorithm>

#include <QtGui/QMainWindow>
#include <QtGui/QApplication>
#include <QtGui/QStringListModel>
#include <QtGui/QMessageBox>

#include <boost/thread.hpp>

#include "ui_client.h"

#define SERVER_PORT 64321
#define CLIENT_PORT 65321
#define MAX_BUFFER 2048

class ClientWindow: public QMainWindow {
Q_OBJECT
public:
  ~ClientWindow();
  static ClientWindow* getInstance();
  void notify(int code);
protected:
  ClientWindow();

  
signals:
  void sig_notify(int code);
protected slots:
  void update_list();
  void update_notify(int code);
  void login();
  void leave();
  void sendmsg();
private:
  Ui::MainWindow *mainwindow;
  static ClientWindow *inst;
  QStringListModel *model;
};

static int client_fd;

static std::string username;
static std::string client_from;
static std::string client_to;

char msg[MAX_BUFFER];

static char socket_buf[MAX_BUFFER];
static char server_buf[MAX_BUFFER];

static std::list<std::string> client_list;


void setusr(const std::string& name) {
  ::username = name;
}

void setsrv(const std::string& server_ip, int server_port) {
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
  server_addr.sin_port = htons(server_port);
  
  if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0) {
    perror("Connecting to server failed!\n");
    exit(-1);
  }
}

void parse_list(char *buf, int len) {
  char *end = buf+len;
  /*
   * the first byte is code 0x11
   */
  ++ buf;
  
  std::string client_name;
  
  while (buf < end) {
    client_name.clear();
    int len = *buf++;
    if (len <= 0) {
      break;
    }
    for (int i = 0; i < len; ++ i) {
      client_name.push_back(*buf++);
    }
    
    ::client_list.push_back(client_name);
  }
}

void login() {
  int i = 0;
  socket_buf[i++] = 0x00; //login
  socket_buf[i++] = ::username.size();
  for (auto x = ::username.begin(); x != ::username.end(); ++ x) {
    socket_buf[i++] = *x;
  }
  while (send(client_fd, socket_buf, i, 0) != i);
}

void leave() {
  int i = 0;
  socket_buf[i++] = 0x01; //leave
  socket_buf[i++] = ::username.size();
  
  for (auto x = ::username.begin(); x != ::username.end(); ++ x) {
    socket_buf[i++] = *x;
  }
  while (send(client_fd, socket_buf, i, 0) != i);
  
  ::client_list.clear();
}

void sendmsg(const std::string client_to, const char *msg, int len) {
  int i = 0;
  
  socket_buf[i++] = 0x02;
  socket_buf[i++] = ::username.size();
  for (auto x = ::username.begin(); x != ::username.end(); ++ x) {
    socket_buf[i++] = *x;
  }
  socket_buf[i++] = client_to.size();
  for (auto x = client_to.begin(); x != client_to.end(); ++ x) {
    socket_buf[i++] = *x;
  }
  socket_buf[i++] = len;
  socket_buf[i] = '\0';
  strcat(socket_buf, msg);
  
  send(client_fd, socket_buf, i+len, 0);
}

void notify(int code) {
  ClientWindow::getInstance()->notify(code);
}

void print_client_list() {
  printf("Client list number: %d\n", client_list.size());
  for (auto x = client_list.begin(); x != client_list.end(); ++ x) {
    printf("%s\n", (*x).c_str());
  }
}

void listen_from_server() {
  char name_from[256], name_to[256];
  while (true) {
    int len = recv(client_fd, server_buf, MAX_BUFFER, 0);
    if (len > 0) {
      int code = server_buf[0];
      int name_from_len = server_buf[1];
      strncpy(name_from, server_buf+2, name_from_len);
      name_from[name_from_len] = '\0';
      
      int name_to_len = server_buf[2+name_from_len];
      strncpy(name_to, server_buf+2+name_from_len+1, name_to_len);
      name_to[name_to_len] = '\0';
      
      char *p = server_buf+2+name_from_len+1+name_to_len;
      
      switch (code) {
	case 0x00: //login
	  client_list.push_back(name_from);
	  notify(0x00);
	  
	  /*
	   * test
	   */
	  printf("client %s has logged in.\n", name_from);
	  print_client_list();
	  break;
	  
	case 0x01: //leave
	{
	  auto new_end = std::remove_if(client_list.begin(), client_list.end(), [&](const std::string& client_from) {
	    return client_from == name_from;
	  });
	  client_list.erase(new_end, client_list.end());
	  notify(0x01);
	  
 	  printf("client %s has left.\n", name_from);
	  print_client_list();
	}
	  break;
	  
	case 0x02: // receive message TODO
	{
	  int len = *p;
	  strncpy(msg, p+1, len);
	  msg[len] = '\0';
	  printf("%s\n", msg);
	  
	  client_from = name_from;
	  client_to = name_to;
	  
	  notify(0x02);
	}
	break;
	  
	case 0x03: // get client list TODO
	{
	  parse_list(server_buf, len);
	  notify(0x03);
	  
	  printf("get client list\n");
	  print_client_list();
	}
	break;
	case 0x04: // invalid login
	{
	  notify(0x04);
	  printf("invalid login.\n");
	}
      }
    }
  } 
}

ClientWindow* ClientWindow::inst = 0;

ClientWindow::ClientWindow() {
  mainwindow = new Ui::MainWindow;
  mainwindow->setupUi(this);
  
  model = new QStringListModel;
  mainwindow->listView->setModel(model);
  connect(this, SIGNAL(sig_notify(int)), this, SLOT(update_notify(int)));
  
  connect(mainwindow->m_btnLogin, SIGNAL(clicked()), this, SLOT(login()));
  connect(mainwindow->m_btnLeave, SIGNAL(clicked()), this, SLOT(leave()));
  connect(mainwindow->m_btnSend, SIGNAL(clicked()), this, SLOT(sendmsg()));
  
  mainwindow->m_btnLeave->setEnabled(false);
} 

ClientWindow::~ClientWindow() {
  delete mainwindow;
  delete model;
}

ClientWindow* ClientWindow::getInstance()
{
  if (inst == 0) {
    inst = new ClientWindow;
  }
  return inst;
}

void ClientWindow::notify(int code)
{
  emit sig_notify(code);
}

void ClientWindow::update_list() {
  QStringList user;
  
  for (auto x = client_list.begin(); x != client_list.end(); ++ x) {
    user += QString::fromStdString(*x);
  }
  this->model->setStringList(user);
}

void ClientWindow::update_notify(int code) {
  switch(code) {
    case 0x00: // another client has logged in
      update_list();
      break;
    case 0x01: // another client has left
      update_list();
      break;
    case 0x02: // receive msg
    {
      QString text;
      text.append("from ");
      text.append(QString::fromStdString(client_from));
      text.append(" to ");
      text.append(QString::fromStdString(client_to));
      text.append(": ");
      text.append(msg);
      mainwindow->msgBox->append(text);
    }
      break;
    case 0x03: // get client list
      update_list();
      mainwindow->m_btnLogin->setEnabled(false);
      mainwindow->m_btnLeave->setEnabled(true);
      break;
    case 0x04: // invalid login
      QMessageBox::critical(0, "Error", "Invalid login");
  }
}

void ClientWindow::login() {
  const QString& text = mainwindow->lineEdit->text();
  setusr(text.toStdString());
  ::login();
}

void ClientWindow::leave()
{
  ::leave();
  update_list();
  mainwindow->lineEdit->clear();
  mainwindow->msgBox->clear();
  mainwindow->msgInputBox->clear();
  mainwindow->m_btnLogin->setEnabled(true);
  mainwindow->m_btnLeave->setEnabled(false);
}

void ClientWindow::sendmsg()
{
  const QString& text = mainwindow->msgInputBox->toPlainText();
  if (!text.isEmpty()) {
    foreach(const QModelIndex &index, mainwindow->listView->selectionModel()->selectedIndexes()) {
      ::sendmsg(model->data(index, Qt::DisplayRole).toString().toStdString().c_str(), text.toStdString().c_str(), text.size());
    }
  }
}

void init(const std::string& server_ip) {
  sockaddr_in client_addr;
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  client_addr.sin_port = htonl(CLIENT_PORT);
  
  if ((::client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("Creating socket for client failed!\n");
      exit(-1);
  };
  setsrv(server_ip, SERVER_PORT);
}

int main(int argc, char *argv[]) {
  init("127.0.0.1");
  
  boost::thread([&](){
    listen_from_server();
  });

  QApplication app(argc, argv);
  ClientWindow::getInstance()->show(); 
  
  return app.exec();
}

#include "client.moc"