#ifndef SERVER_H_
#define SERVER_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <mutex>
#include <thread>
#include <vector>

class Server {
 public:
  Server(int port);
  ~Server();

  int Socket(int domain, int type, int protocol);
  void Bind(int sock_fd, const sockaddr* addr, socklen_t len);
  void Listen(int sock_fd, int backlog_size);
  int Accept(int sock_fd, struct sockaddr* addr, socklen_t* len);
  void Sum(int num);
  void HandleClient(int conn_fd);
  void RunServer(void);

  int listener_fd_, conn_fd_;
  struct sockaddr_in addr_;
  socklen_t addr_size_;

  int num_, total_;
  int num_clients_;
  std::mutex sum_mutex_;
  std::mutex num_client_mutex_;
  std::vector<int> clients_;
};

#endif  // SERVER_H_