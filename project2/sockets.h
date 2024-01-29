#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <vector>
#include <string>

int Socket(int domain, int type, int protocol);
void Bind(int sock_fd, const sockaddr* addr, socklen_t len);
void Listen(int sock_fd, int backlog_size);
int Accept(int sock_fd, struct sockaddr* addr, socklen_t* len);
void Connect(int sock_fd, struct sockaddr *addr, socklen_t len);
void Close(int sock_fd);


int Server(struct sockaddr_in addr, socklen_t addr_size, int port);
int Client(std::string host, int port);

#endif // SOCKETS_H_
