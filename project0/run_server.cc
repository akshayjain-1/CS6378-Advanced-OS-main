#include <chrono>

#include "server.h"

int main(int argc, char* argv[]) {
  Server* server = new Server(8080);
  std::thread server_thread(&Server::RunServer, server);
  server_thread.join();
  return 0;
}