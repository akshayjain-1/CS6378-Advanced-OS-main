#include "client.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <random>
#include <string>

Client::Client(const char* host, int port) {
  memset(&this->addr_, 0, sizeof(addr_));
  this->addr_.sin_family = AF_INET;
  this->addr_.sin_port = htons(port);
  this->addr_size_ = sizeof(addr_);
  inet_pton(AF_INET, host, &this->addr_.sin_addr);

  this->conn_fd_ = Socket(AF_INET, SOCK_STREAM, 0);
  printf("\nClient: connecting to %s:%d\n", host, port);
}

int Client::Socket(int domain, int type, int protocol) {
  int sock_fd;
  if ((sock_fd = socket(domain, type, protocol)) == -1) {
    perror("socket");
    exit(1);
  }
  return sock_fd;
}

void Client::Connect(int sock_fd, struct sockaddr* addr, socklen_t len) {
  if (connect(sock_fd, addr, len) == -1) {
    perror("connect");
    exit(1);
  }
}

std::pair<int, int> Client::GetRandomNums(void) {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> sleep_time(
      2, 16);  // distribution in range [2, 16]
  std::uniform_int_distribution<std::mt19937::result_type> dist(
      0, 1000);  // distribution in range [0, 1000]
  std::pair<int, int> random_nums = {(int)sleep_time(rng), (int)dist(rng)};
  return random_nums;
}

void Client::HandleConnection(void) {
  std::this_thread::yield();
  for (int i = 0; i < 1; i++) {
    std::pair<int, int> random_nums = GetRandomNums();
    std::this_thread::sleep_for(std::chrono::seconds(random_nums.first));

    std::string msg = std::to_string(random_nums.second);
    int msg_len = msg.length();

    printf("Message %s sent with length %d\n", msg.c_str(), msg_len);
    // send message length
    send(this->conn_fd_, std::to_string(msg_len).c_str(), 1, 0);
    // send message
    send(this->conn_fd_, msg.c_str(), msg_len, 0);
  }
}

void Client::RunClient(void) {
  Connect(this->conn_fd_, (struct sockaddr*)&this->addr_, this->addr_size_);

  HandleConnection();

  close(this->conn_fd_);
}

int main(int argc, char* argv[]) {
  Client* client = new Client("192.168.56.102", 8080);
  std::thread client_thread(&Client::RunClient, client);
  client_thread.join();
  return 0;
}