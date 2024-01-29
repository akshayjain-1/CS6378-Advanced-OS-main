#include "sockets.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <thread>
#include <chrono>


int Socket(int domain, int type, int protocol) {
  int sock_fd;
  if ((sock_fd = socket(domain, type, protocol)) == -1) {
    perror("socket");
    exit(1);
  }
  return sock_fd;
}

void Bind(int sock_fd, const sockaddr *addr, socklen_t len) {
  int yes = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
  if (bind(sock_fd, addr, len) == -1) {
    perror("bind");
    exit(1);
  }
}

void Listen(int sock_fd, int backlog_size) {
  if (listen(sock_fd, backlog_size) == -1) {
    perror("listen");
    exit(1);
  }
}

int Accept(int sock_fd, struct sockaddr *addr, socklen_t *len) {
  int conn_fd;
  if ((conn_fd = accept(sock_fd, addr, len)) == -1) {
    perror("accept");
    exit(1);
  }
  return conn_fd;
}

void Connect(int sock_fd, struct sockaddr *addr, socklen_t len) {
  // keep trying till connection is established
  if (connect(sock_fd, addr, len) == -1) {
    perror("accept");
    exit(1);
  }
}

void Close(int sock_fd) {
  shutdown(sock_fd, SHUT_WR);
  close(sock_fd);
}

int Client(std::string host, int port) {
  struct hostent *he;
  if ((he = gethostbyname(host.c_str())) == NULL) {
    perror("gethostbyname");
    exit(1);
  }

  socklen_t server_addr_size;
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr_size = sizeof(server_addr);
  memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

  int conn_fd = Socket(AF_INET, SOCK_STREAM, 0);
  Connect(conn_fd, (sockaddr *)&server_addr, sizeof(server_addr));
  printf("Client %d: connected to %s:%d\n", conn_fd, host.c_str(), port);

  return conn_fd;
}

int Server(struct sockaddr_in addr, socklen_t addr_size, int port) {
  int listener_fd = Socket(AF_INET, SOCK_STREAM, 0);
  Bind(listener_fd, (struct sockaddr *)&addr, addr_size);
  Listen(listener_fd, 5);

  char server_addr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &addr.sin_addr, server_addr, sizeof(server_addr));
  printf("Server: Listening on %s:%d\n", server_addr, port);

  return listener_fd;
}