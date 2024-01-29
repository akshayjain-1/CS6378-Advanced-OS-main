#include "server.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <string>

Server::Server(int port) {
  memset(&this->addr_, 0, sizeof(addr_));
  this->addr_.sin_family = AF_INET;
  this->addr_.sin_addr.s_addr = htonl(INADDR_ANY);
  this->addr_.sin_port = htons(port);
  this->addr_size_ = sizeof(addr_);

  this->listener_fd_ = Socket(AF_INET, SOCK_STREAM, 0);
  Bind(this->listener_fd_, (struct sockaddr*)&this->addr_, this->addr_size_);
  Listen(this->listener_fd_, 5);

  char server_addr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &this->addr_.sin_addr, server_addr, sizeof(server_addr));
  printf("Server:  Listening on %s:%d\n", server_addr, port);

  this->num_clients_ = 0;
}

int Server::Socket(int domain, int type, int protocol) {
  int sock_fd;
  if ((sock_fd = socket(domain, type, protocol)) == -1) {
    perror("socket");
    exit(1);
  }
  return sock_fd;
}

void Server::Bind(int sock_fd, const sockaddr* addr, socklen_t len) {
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

void Server::Listen(int sock_fd, int backlog_size) {
  if (listen(sock_fd, backlog_size) == -1) {
    perror("listen");
    exit(1);
  }
}

int Server::Accept(int sock_fd, struct sockaddr* addr, socklen_t* len) {
  int conn_fd;
  if ((conn_fd = accept(sock_fd, addr, len)) == -1) {
    perror("accept");
    exit(1);
  }
  return conn_fd;
}

void Server::Sum(int num) {
  this->sum_mutex_.lock();
  this->total_ += num;
  std::cout << "Thread id: " << std::this_thread::get_id()
            << " total = " << this->total_ << std::endl;
  /* printf("\nThread id: %i, total = %d\n", std::this_thread::get_id(),
         this->total_);
         this->sum_mutex_.unlock();
         */
  this->sum_mutex_.unlock();
}

void Server::HandleClient(int conn_fd) {
  std::cout << "NEW Thread id created: " << std::this_thread::get_id()
            << std::endl;

  char buf[1024];
  int msg_len, client_num;
  for (int i = 0; i < 5; i++) {
    /*
    memset(&buf, 0, sizeof(buf));
    if (recv(this->conn_fd_, buf, 200, 0) == -1) {
      perror("server recv");
      exit(1);
    }
    printf("Received msg_len = %s\n", buf);
    msg_len = std::stoi(buf);
*/
    std::cout << "Thread id: " << std::this_thread::get_id()
              << " waiting to receive" << std::endl;

    memset(&buf, 0, sizeof(buf));
    if (recv(conn_fd, buf, 200, 0) == -1) {
      perror("server recv");
      exit(1);
    }
    std::cout << "Thread id: " << std::this_thread::get_id()
              << " received = " << buf << std::endl;
    client_num = std::stoi(buf);

    Sum(client_num);
  }
  this->num_client_mutex_.lock();
  this->num_clients_++;
  std::cout << "Thread id: " << std::this_thread::get_id()
            << " num_clients = " << this->num_clients_ << std::endl;
  /*j
  printf("\nThread id: %d, num_clients =  %d\n", std::this_thread::get_id(),
         this->num_clients_);
         */
  this->num_client_mutex_.unlock();
  // close(this->conn_fd_);
}

void Server::RunServer(void) {
  char client_addr[INET_ADDRSTRLEN];
  std::vector<std::thread> client_threads;
  std::vector<int> client_fds;

  while (this->num_clients_ != 2) {
    /// printf("\nnum_clients = %d\n", this->num_clients_);
    int conn_fd = Accept(this->listener_fd_, (struct sockaddr*)&this->addr_,
                         &this->addr_size_);

    client_fds.push_back(conn_fd);
    // printf("AFTER ACCEPT\n");
    inet_ntop(AF_INET, &this->addr_.sin_addr, client_addr, sizeof(client_addr));
    printf("\nServer: Received connection from %s\n", client_addr);

    client_threads.emplace_back(&Server::HandleClient, this, conn_fd);

    for (auto& client_thread : client_threads) {
      if (client_thread.joinable()) {
        client_thread.detach();
      }
    }
  }

  /*
  printf("\nOUTSIDE WHILE\n");
  for (auto& client_thread : client_threads) {
    if (client_thread.joinable()) {
      printf("Thread id: %d, is joinable\n", client_thread.get_id());
      client_thread.join();
    }
  }
    */

  printf("\ntotal = %d\n", this->total_);
  close(this->listener_fd_);
}

int main(int argc, char* argv[]) {
  Server* server = new Server(8080);
  std::thread server_thread(&Server::RunServer, server);
  server_thread.join();
  return 0;
}