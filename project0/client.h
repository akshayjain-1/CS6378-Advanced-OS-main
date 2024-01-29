
#ifndef CLIENT_H_
#define CLIENT_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <mutex>
#include <thread>
#include <utility>
#include <vector>

class Client {
 public:
  Client(const char* host, int port);
  ~Client();

  int Socket(int domain, int type, int protocol);
  void Connect(int sock_fd, struct sockaddr* addr, socklen_t len);
  void HandleConnection(void);
  void RunClient(void);

  std::pair<int, int> GetRandomNums(void);

  int conn_fd_;
  struct sockaddr_in addr_;
  socklen_t addr_size_;

  int num_, total_;
  std::mutex sumMutex_;
  std::vector<int> clients_;
};

#endif  // CLIENT_H_