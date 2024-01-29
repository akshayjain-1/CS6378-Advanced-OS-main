#include <chrono>

#include "client.h"

int main(int argc, char* argv[]) {
  Client* client = new Client("192.168.56.102", 8080);
  std::thread client_thread(&Client::RunClient, client);

  client_thread.join();
  return 0;
}