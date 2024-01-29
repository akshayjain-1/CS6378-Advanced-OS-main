#include "node.h"

#include <netdb.h>
#include <string.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include "message.h"
#include "sockets.h"
#include "utility.h"

using namespace std;

bool QueueComparator::operator()(const std::pair<int, int> &q1,
                                 const std::pair<int, int> &q2) {
  if (q1.first != q2.first)
    return q1.first > q2.first;
  return q1.second > q2.second;
}

Node::Node(const std::string &hostname, const int &port,
           const std::vector<std::string> &input) {
  this->hostname = hostname;
  this->port = port;
  this->current_state = IDLE;
  this->timestamp = 0;

  vector<string> result = split(input[0], ' ');
  this->num_nodes = stoi(result[0]);
  this->inter_request_delay = stoi(result[1]);
  this->cs_execution_time = stoi(result[2]);
  this->total_num_requests = stoi(result[3]);
  this->num_grants_received = 0;

  this->conn_fds.assign(this->num_nodes, 0);
};

void Node::SetNodes(const vector<string> &input) {
  vector<string> result;
  int neighbour_fd;
  for (int i = 1; i <= this->num_nodes; i++) {
    result = split(input[i], ' ');
    if (result[1] == this->hostname)
      this->id = i - 1;
    this->nodes.emplace_back(result[1], stoi(result[2]));
    if (stoi(result[0]) != this->id) {
      int neighbour_fd = Client(result[1], stoi(result[2]));
      this->conn_fds[stoi(result[0])] = neighbour_fd;
    }
  }
}

void Node::SendMessage(const int &conn_fd, const std::string &msg,
                       const int &num_bytes) {
  if (send(conn_fd, msg.c_str(), num_bytes, 0) == 0) {
    perror("send");
  }
}

std::string Node::ReceiveMessage(const int &conn_fd, const int &num_bytes) {
  char buf[1024];
  if (recv(conn_fd, buf, num_bytes, 0) == 0) {
    perror("server recv");
    return NULL;
  }
  return string(buf);
}

int Node::GetCurrentTimestamp(void) {
  lock_guard<mutex> timestamp_lck(this->timestamp_mtx);
  return this->timestamp;
}

void Node::UpdateTimestamp(void) {
  lock_guard<mutex> timestamp_lck(this->timestamp_mtx);
  this->timestamp++;
}

void Node::UpdateTimestamp(const int &request_timestamp) {
  lock_guard<mutex> timestamp_lck(this->timestamp_mtx);
  this->timestamp = max(this->timestamp, request_timestamp) + 1;
}

int Node::GetCurrentState(void) {
  lock_guard<mutex> current_state_lck(this->current_state_mtx);
  return this->current_state;
}

void Node::UpdateCurrentState(const system_states &new_state) {
  lock_guard<mutex> current_state_lck(this->current_state_mtx);
  this->current_state = new_state;
}

int Node::GetGrantsReceived(void) {
  lock_guard<mutex> num_grants_received_lck(this->num_grants_received_mtx);
  return this->num_grants_received;
}

void Node::UpdateGrantsReceived(void) {
  lock_guard<mutex> num_grants_received_lck(this->num_grants_received_mtx);
  this->num_grants_received++;
}

void Node::ResetGrantsReceived(void) {
  lock_guard<mutex> num_grants_received_lck(this->num_grants_received_mtx);
  this->num_grants_received = 0;
}

int Node::GetNumRequests(void) {
  lock_guard<mutex> num_requests_lck(this->num_requests_mtx);
  return this->num_requests;
}

void Node::UpdateNumRequests(void) {
  lock_guard<mutex> num_requests_lck(this->num_requests_mtx);
  this->num_requests++;
}

bool Node::RequestQueueIsEmpty(void) {
  lock_guard<mutex> request_queue_lck(this->request_queue_mtx);
  return this->request_queue.empty();
}

void Node::RequestQueuePush(const int &timestamp, const int &id) {
  lock_guard<mutex> request_queue_lck(this->request_queue_mtx);
  this->request_queue.emplace(timestamp, id);
}

void Node::RequestQueuePop(void) {
  lock_guard<mutex> request_queue_lck(this->request_queue_mtx);
  if (!this->request_queue.empty())
    this->request_queue.pop();
}

pair<int, int> Node::RequestQueueTop(void) {
  lock_guard<mutex> request_queue_lck(this->request_queue_mtx);
  if (this->request_queue.empty())
    return {};
  return this->request_queue.top();
}

pair<int, int> Node::GetRequestById(const int &id) {
  priority_queue<pair<int, int>, vector<pair<int, int>>, QueueComparator> queue;
  {
    lock_guard<mutex> reply_queue_lck(this->request_queue_mtx);
    queue =
        priority_queue<pair<int, int>, vector<pair<int, int>>, QueueComparator>(
            this->request_queue);
  }
  pair<int, int> request;

  while (!queue.empty()) {
    request = queue.top();
    if (request.second == id)
      break;
    queue.pop();
  }
  return request;
}

void Node::RequestQueueRemove(const int &id) {
  lock_guard<mutex> request_queue_lck(this->request_queue_mtx);
  priority_queue<pair<int, int>, vector<pair<int, int>>, QueueComparator> queue;
  pair<int, int> top_of_queue;

  while (!this->request_queue.empty()) {
    top_of_queue = this->request_queue.top();
    if (top_of_queue.second != id)
      queue.emplace(top_of_queue);
    this->request_queue.pop();
  }
  this->request_queue = queue;
}

string Node::RequestQueueToString(void) {
  priority_queue<pair<int, int>, vector<pair<int, int>>, QueueComparator> queue;
  {
    lock_guard<mutex> reply_queue_lck(this->request_queue_mtx);
    queue =
        priority_queue<pair<int, int>, vector<pair<int, int>>, QueueComparator>(
            this->request_queue);
  }
  printf("INSIDE TO STRING\n");
  string result = "[";
  string temp = "";
  while (!queue.empty()) {
    temp += " (" + to_string(queue.top().first) + "," +  to_string(queue.top().second) + ")";
    result += temp;
    queue.pop();
  }
  result += " ]";
  return result;
}

void Node::DisplayRequestQueue(void) {
  priority_queue<pair<int, int>, vector<pair<int, int>>, QueueComparator> queue;
  {
    lock_guard<mutex> reply_queue_lck(this->request_queue_mtx);
    queue =
        priority_queue<pair<int, int>, vector<pair<int, int>>, QueueComparator>(
            this->request_queue);
  }
  printf("Node: %d, Request Queue: [", this->id);
  while (!queue.empty()) {
    printf(" (%d, %d)", queue.top().first, queue.top().second);
    queue.pop();
  }
  printf(" ]\n");
}

void Node::SendRequestMsg(void) {
  UpdateTimestamp();
  RequestQueuePush(this->timestamp, this->id);
  DisplayRequestQueue();

  struct Message m =
      ConstructMessage("Request", GetCurrentTimestamp(), this->id);
  string msg;
  int conn_fd;

  printf("\nSending Request Messages\n");
  for (int i = 0; i < this->num_nodes; i++) {
    if (i != this->id) {
      conn_fd = this->conn_fds[i];
      msg = SerializeMessage(m);
      printf("Client %d: %s\n", conn_fd, msg.c_str());
      SendMessage(conn_fd, msg, 512);
    }
  }
}

void Node::SendGrantMsg(const int &conn_fd) {
  struct Message m = ConstructMessage("Grant", GetCurrentTimestamp(), this->id);
  string msg = SerializeMessage(m);

  printf("\nSending Grant Message\n");
  printf("Client %d: %s\n", conn_fd, msg.c_str());
  SendMessage(conn_fd, msg, 512);
}

void Node::SendReleaseMsg(void) {
  struct Message m =
      ConstructMessage("Release", GetCurrentTimestamp(), this->id);
  string msg = SerializeMessage(m);

  UpdateTimestamp();
  DisplayRequestQueue();
  printf("Sending Release Messages\n");
  int conn_fd;
  for (int i = 0; i < this->num_nodes; i++) {
    if (i != this->id) {
      conn_fd = this->conn_fds[i];
      msg = SerializeMessage(m);
      printf("Client %d: %s\n", conn_fd, msg.c_str());
      SendMessage(conn_fd, msg, 512);
    }
  }
}

void Node::HandleServerConnection(const int &conn_fd,
                                  const std::string &peer_name) {

  string msg;
  struct Message m;
  while (true) {
    string msg = ReceiveMessage(conn_fd, 512);
    struct Message m = DeserializeMessage(msg);
    printf("\nTimestamp: %d, Current State (IDLE:1, WAITING:2, "
           "CRITICAL:3) : %d\n",
           GetCurrentTimestamp(), GetCurrentState());
    printf("\nServer %d: received: %s\n", conn_fd, msg.c_str());
    UpdateTimestamp(m.timestamp);

    if (m.identifier == "Request") {
      RequestQueuePush(m.timestamp, m.id);
      SendGrantMsg(this->conn_fds[m.id]);
    } else if (m.identifier == "Release") {
      RequestQueueRemove(m.id);
    } else if (m.identifier == "Grant") {
      UpdateGrantsReceived();
    }
    DisplayRequestQueue();
  }
}

void Node::RunServer(const int &port) {
  struct sockaddr_in addr;
  socklen_t addr_size;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  addr_size = sizeof(addr);

  int listener_fd = Server(addr, addr_size, port);

  vector<thread> client_threads;
  struct hostent *he;
  string peer_name;

  while (true) {
    int conn_fd = Accept(listener_fd, (struct sockaddr *)&addr, &addr_size);

    he = gethostbyaddr(&addr.sin_addr.s_addr, sizeof(addr.sin_addr.s_addr),
                       AF_INET);
    peer_name = split(string(he->h_name), '.')[0];
    client_threads.emplace_back(&Node::HandleServerConnection, this, conn_fd,
                                peer_name);
  }
  for (auto &client_thread : client_threads)
    client_thread.join();
}

bool Node::L1(void) {
  // check if all grants are received
  if (GetGrantsReceived() == this->num_nodes - 1)
    return true;
  return false;
}

bool Node::L2(void) {
  // check if my request is at top of queue
  pair<int, int> my_request = RequestQueueTop();
  if (!RequestQueueIsEmpty() && my_request.second == this->id) {
    return true;
  }
  return false;
}

void Node::CSEnter(double error_threshold) {
  printf("\nNode %d: is requesting to enter critical section\n", this->id);
  SendRequestMsg();

  // block till process can enter critical section
  UpdateCurrentState(WAITING);
  while (GetCurrentState() == WAITING) {
    if (error_threshold > 8 && L1()) {
      UpdateCurrentState(CRITICAL);
    } else if (L2() && L1())
      UpdateCurrentState(CRITICAL);
  }
}

void Node::CSExecute(double execution_time) {
  printf("\nNode %d: Current State (IDLE:1, WAITING:2, "
         "CRITICAL:3) : %d\n",
         this->id, GetCurrentState());

  pair<int, int> my_request = GetRequestById(this->id);
  
  // output_log format
  // top_request_timestamp, top_request_id, Start/End
  const string file_name = "output_log.csv";
  string result = to_string(my_request.first) + "," + to_string(my_request.second) + ",Start";
  write_file(file_name, result);

  this_thread::sleep_for(chrono::duration<double, milli>(execution_time));

  RequestQueueRemove(this->id);

  result = to_string(my_request.first) + "," + to_string(my_request.second) + ",End";
  write_file(file_name, result);

}

void Node::CSLeave(double execution_time, double request_delay) {
  printf("\nNode %d: is leaving critical section\n", this->id);

  // broadcast all release messages to my neighbours
  SendReleaseMsg();
  ResetGrantsReceived();
  UpdateCurrentState(IDLE);
}

void Node::RunClient(bool check_error) {
  double request_delay;
  double error_threshold;
  int current_state;
  int i;

  double execution_time = exponential_distribution(this->cs_execution_time);
  double response_time;
  double total_request_delay = 0.0, total_cs_execution_time = 0.0;

  auto start = chrono::steady_clock::now();
  while (GetNumRequests() < this->total_num_requests) {
    if (GetCurrentState() == IDLE) {
      error_threshold = check_error ? exponential_distribution(1) : 0;

      auto start = chrono::steady_clock::now();
      request_delay = this->inter_request_delay ? exponential_distribution(this->inter_request_delay) : 0;
      total_request_delay += request_delay;
      this_thread::sleep_for(chrono::duration<double, milli>(request_delay));

      // broadcast request messages to neighbours and check that L2 and L1 are satisfied
      CSEnter(error_threshold);
      auto end = chrono::steady_clock::now();

      execution_time = this->cs_execution_time ? exponential_distribution(this->cs_execution_time) : 0;
      total_cs_execution_time += execution_time;
      CSExecute(execution_time);
      response_time = chrono::duration<double, milli>(end - start).count() + execution_time;

      string result = to_string(response_time) + "," + to_string(execution_time);
      write_file("response_time"+ to_string(this->id) + ".csv", result);

      CSLeave(execution_time, request_delay);

      UpdateNumRequests();
    }
  }
  auto end = chrono::steady_clock::now();
  double total_time = chrono::duration<double, milli>(end - start).count();
  double system_throughput = GetNumRequests() / total_time;

  
  string result = to_string(system_throughput) + "," + to_string(total_cs_execution_time / 1000);
  write_file("system_throughput"+ to_string(this->id) + ".csv", result);

  printf("\nNode %d: done num_requests: %d, total_num_requests: %d\n", this->id,
         GetNumRequests(), this->total_num_requests);
  Close(this->conn_fds[this->id]);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: ./node <host> <port>\n");
    exit(1);
  }
  vector<string> lines = read_file("config.txt");

  string host(argv[1]);
  int port = stoi(argv[2]);

  Node *node = new Node(host, port, lines);

  thread server_thread(&Node::RunServer, node, node->port);

  this_thread::sleep_for(chrono::seconds(10));
  node->SetNodes(lines);
  printf("\nNode %d: %s %d\n", node->id, node->hostname.c_str(), node->port);

  this_thread::sleep_for(chrono::seconds(10));
  thread client_thread(&Node::RunClient, node, false);

  client_thread.join();
  server_thread.join();
  return 0;
}
