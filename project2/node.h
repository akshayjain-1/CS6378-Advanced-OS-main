#ifndef NODE_H_
#define NODE_H_

#include <mutex>
#include <queue>
#include <string>
#include <utility>
#include <vector>

enum system_states { IDLE = 1, WAITING = 2, CRITICAL = 3 };

class QueueComparator {
public:
  bool operator()(const std::pair<int, int> &q1, const std::pair<int, int> &q2);
};

class Node {
public:
  Node(const std::string &hostname, const int &port,
       const std::vector<std::string> &input);
  void SetNodes(const std::vector<std::string> &input);
  void Setconn_fds(const std::vector<std::string> &input);

  int NumberOfMessagesToSend(void);
  void SendMessage(const int &conn_fd, const std::string &msg,
                   const int &num_bytes);
  std::string ReceiveMessage(const int &conn_fd, const int &num_bytes);

  int GetCurrentTimestamp(void);
  void UpdateTimestamp();
  void UpdateTimestamp(const int &request_timestamp);

  int GetCurrentState(void);
  void UpdateCurrentState(const system_states &new_state);

  int GetNumRequests(void);
  void UpdateNumRequests(void);
  
  int GetGrantsReceived(void);
  void UpdateGrantsReceived(void);
  void ResetGrantsReceived(void);

  bool RequestQueueIsEmpty(void);
  void RequestQueuePush(const int &timestamp, const int &id);
  void RequestQueuePop(void);
  std::pair<int, int> RequestQueueTop(void);
  void RequestQueueRemove(const int &id);
  std::string RequestQueueToString(void);
  void DisplayRequestQueue(void);

  std::pair<int, int> GetRequestById(const int &id);

  void SendRequestMsg(void);
  void SendGrantMsg(const int &conn_fd);
  void SendReleaseMsg(void);

  bool L1(void);
  bool L2(void);
  void CSEnter(double error_threshold);
  void CSExecute(double response_time);
  void CSLeave(double execution_time, double request_delay);

  void HandleServerConnection(const int &conn_fd, const std::string &peer_name);
  void RunServer(const int& port);
  void RunClient(bool check_error);

  int id;
  std::string hostname;
  int port;

  int num_nodes, inter_request_delay, cs_execution_time, total_num_requests;
  std::vector<int> conn_fds;

private:
  std::vector<std::pair<std::string, int>> nodes;
  int timestamp;
  std::mutex timestamp_mtx;

  bool is_in_critical_section;
  std::mutex is_in_critical_section_mtx;

  int num_requests;
  std::mutex num_requests_mtx;

  // priority queue that holds {request_timestamp, id} pairs
  std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>,
                      QueueComparator>
      request_queue;
  std::mutex request_queue_mtx;
  
  int num_grants_received;
  std::mutex num_grants_received_mtx;

  system_states current_state;
  std::mutex current_state_mtx;
};

#endif // NODE_H_