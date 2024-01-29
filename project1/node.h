#ifndef NODE_H_
#define NODE_H_

#include <string>
#include <utility>
#include <vector>
#include <mutex>
#include <tuple>

class Node {
public:
  Node(std::string hostname, int port, std::vector<std::string> &input);

  void SetNodes(std::vector<std::string> &input);
  void SetNeighbours(std::vector<std::string> &input);
  std::pair<int, int> PickRandomNeighbour(void);

  void DisplayStatus(void);
  void SetPassive(void);
  void SetActive(void);
  void SetActive(int id);

  int NumberOfMessagesToSend(void);

  void SendMessage(int conn_fd, int num);
  void SendMessage(int conn_fd, std::string msg, int num_bytes);
  int ReceiveMessage(int conn_fd);
  std::string ReceiveMessage(int conn_fd, int num_bytes);

  void DisplayTimestamps(std::string thread_name);
  void UpdateTimestamps();
  void UpdateTimestamps(std::vector<int> peer_timestamps);
  std::vector<int> StringToTimestamp(std::string timestamp);
  std::string TimestampToString(std::vector<int> timestamp);

  void HandleServerConnection(int conn_fd, std::string peer_name);
  void HandleClientConnection(int conn_fd);

  void RunServer(int port);
  void RunClient(void);

  void ChandyLamport(void);
  void SendMarkerMessages(void);
  void ReceiveMarkerMessages(bool& is_first_marker, struct Message m, int& num_markers_received, std::string msg);
  void IncrementIncomingChannels(void);
  void IncrementMarkersReceived(void);
  bool IsFirstMarker(void);
  void AddMessageToChannelStates(int channel_id, std::string msg);
  void StartRecordingChannels(int channel_id);
  void StopRecordingChannel(int channel_id);

  struct Message ConstructMessage(std::string identifier, const std::vector<int>& timestamp, int value);
  std::string SerializeMessage(struct Message m);
  struct Message DeserializeMessage(std::string msg);

  void DisplayChannelStates(void);

  int id;
  std::string hostname;
  int port;
  bool is_active;

  int num_nodes, min_per_active, max_per_active, min_send_delay, snapshot_delay, max_number;

private:
  int total_messages;
  std::mutex total_messages_mtx;

  std::vector<std::pair<std::string, int>> nodes;
  std::vector<std::pair<int, int>> neighbours; // holds neighbour_id and neighbour_conn_fd

  std::mutex is_active_mtx;

  std::vector<int> timestamps;
  std::mutex timestamp_mtx;

  std::vector<std::vector<std::string>> channel_states;
  std::mutex channel_states_mtx;

  std::vector<bool> channel_is_recording;
  std::mutex channel_is_recording_mtx;

  int num_incoming_channels;
  std::mutex num_incoming_channels_mtx;

  int num_markers_received;
  std::mutex num_markers_received_mtx;

  bool is_first_marker = false;
  std::mutex is_first_marker_mtx;
  
};

#endif // NODE_H_