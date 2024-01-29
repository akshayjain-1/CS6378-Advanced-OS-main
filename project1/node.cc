#include "node.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "client.h"
#include "server.h"
#include "utility.h"

using namespace std;

mutex message_mtx;
// [A/M | <vector timestamp T> | Value (V)]
struct Message {
  string identifier;
  vector<int> timestamp;
  int value;
};

struct Message Node::ConstructMessage(string identifier,
                                      const vector<int> &timestamp, int value) {
  struct Message m;
  m.identifier = identifier;
  m.timestamp = timestamp;
  m.value = value;

  return m;
}

string Node::TimestampToString(vector<int> timestamp) {
  string result = "";
  int i = 0;
  while (i < timestamp.size() - 1) {
    result += to_string(timestamp[i]) + " ";
    i++;
  }
  result += to_string(timestamp[i]);
  return result;
}

vector<int> Node::StringToTimestamp(string timestamp) {
  vector<int> t(this->num_nodes, 0);
  vector<string> result = split(timestamp, ' ');
  for (int i = 0; i < result.size(); i++) {
    t[i] = stoi(result[i]);
  }
  return t;
}

string Node::SerializeMessage(struct Message m) {
  string msg = "";
  msg += m.identifier + "|";
  msg += TimestampToString(m.timestamp) + "|";
  msg += to_string(m.value);

  return msg;
}

struct Message Node::DeserializeMessage(string msg) {
  struct Message m;
  vector<string> result = split(msg, '|');
  m.identifier = result[0];
  m.timestamp = StringToTimestamp(result[1]);
  m.value = stoi(result[2]);

  return m;
}

Node::Node(string hostname, int port, vector<string> &input) {
  this->hostname = hostname;
  this->port = port;
  this->is_active = false;
  this->total_messages = 0;

  vector<string> result = split(input[0], ' ');
  this->num_nodes = stoi(result[0]);
  this->min_per_active = stoi(result[1]);
  this->max_per_active = stoi(result[2]);
  this->min_send_delay = stoi(result[3]);
  this->snapshot_delay = stoi(result[4]);
  this->max_number = stoi(result[5]);

  this->timestamps.assign(this->num_nodes, 0);
  SetNodes(input);

  this->num_incoming_channels = 0;
  this->num_markers_received = 0;
  this->channel_states.assign(this->num_nodes, {}); // X = empty
  this->channel_is_recording.assign(this->num_nodes, false);
};

void Node::SetNodes(vector<string> &input) {
  vector<string> result;
  for (int i = 1; i <= this->num_nodes; i++) {
    result = split(input[i], ' ');
    if (result[1] == this->hostname)
      this->id = i - 1;
    this->nodes.emplace_back(result[1], stoi(result[2]));
  }
}

void Node::SetNeighbours(vector<string> &input) {
  int neighbour_id;
  pair<string, int> neighbour;

  struct hostent *he;
  struct sockaddr_in client_addr;

  int n = input.size();
  vector<string> result;

  for (int i = this->num_nodes + 1; i < n; i++) {
    if (i % (this->num_nodes + 1) == this->id) {
      result = split(input[i], ' ');
      for (auto j : result) {
        neighbour_id = stoi(j);
        neighbour = this->nodes[neighbour_id];
        if ((he = gethostbyname(neighbour.first.c_str())) == NULL) {
          perror("gethostbyname");
          exit(1);
        }

        memset(&client_addr, 0, sizeof(client_addr));
        memcpy(&client_addr.sin_addr, he->h_addr_list[0], he->h_length);
        client_addr.sin_port = htons(neighbour.second);
        client_addr.sin_family = AF_INET;

        int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
        connect(conn_fd, (struct sockaddr *)&client_addr, sizeof(client_addr));
        this->neighbours.emplace_back(neighbour_id, conn_fd);

        printf("Client %d: connected to %s:%d\n", conn_fd,
               neighbour.first.c_str(), neighbour.second);
      }
      break;
    }
  }
}
void Node::DisplayStatus(void) {
  printf("Node_id: %d, is_active: %s\n", this->id,
         this->is_active ? "true" : "false");
}

void Node::SetPassive(void) {
  lock_guard<mutex> is_active_lock(this->is_active_mtx);
  this->is_active = false;
}

void Node::SetActive(void) {
  lock_guard<mutex> is_active_lock(this->is_active_mtx);
  this->is_active = true;
}
void Node::SetActive(int id) {
  if (id != this->id)
    return;
  SetActive();
}

void Node::SendMessage(int conn_fd, int num) {
  uint32_t network_byte_order = htonl(num);
  if (send(conn_fd, &network_byte_order, sizeof(uint32_t), 0) == -1) {
    perror("send");
  }
}

void Node::SendMessage(int conn_fd, string msg, int num_bytes) {
  if (send(conn_fd, msg.c_str(), num_bytes, 0) == -1) {
    perror("send");
  }
}

int Node::ReceiveMessage(int conn_fd) {
  uint32_t num;
  if (recv(conn_fd, &num, sizeof(uint32_t), 0) == -1) {
    perror("server recv");
    return -1;
  }
  return ntohl(num);
}

string Node::ReceiveMessage(int conn_fd, int num_bytes) {
  char buf[1024];
  if (recv(conn_fd, buf, num_bytes, 0) == -1) {
    perror("server recv");
    return NULL;
  }
  return string(buf);
}

void Node::DisplayTimestamps(string thread_name) {
  int i = 0;
  printf("\n%s: timestamps: <", thread_name.c_str());
  while (i < this->timestamps.size() - 1) {
    printf("%d, ", this->timestamps[i]);
    i++;
  }
  printf("%d>\n", this->timestamps[i]);
}

void Node::UpdateTimestamps() {
  lock_guard<mutex> timestamp_lck(this->timestamp_mtx);
  this->timestamps[this->id]++;
}

void Node::UpdateTimestamps(vector<int> peer_timestamps) {
  for (int i = 0; i < this->num_nodes; i++) {
    lock_guard<mutex> timestamp_lck(this->timestamp_mtx);
    this->timestamps[i] = max(this->timestamps[i], peer_timestamps[i]);
  }
  UpdateTimestamps();
}

void Node::IncrementIncomingChannels(void) {
  lock_guard<mutex> incoming_channels_lck(this->num_incoming_channels_mtx);
  this->num_incoming_channels++;
}

void Node::IncrementMarkersReceived(void) {
  lock_guard<mutex> markers_received_lck(this->num_markers_received_mtx);
  this->num_markers_received++;
}

bool Node::IsFirstMarker(void) {
  return (this->id != 0 && this->num_markers_received == 1) ? true : false;
}

void Node::StopRecordingChannel(int channel_id) {
  unique_lock<mutex> channel_is_recording_lck(this->channel_is_recording_mtx);
  this->channel_is_recording[channel_id] = false;
}

void Node::StartRecordingChannels(int channel_id) {
  for (int i = 0; i < this->channel_is_recording.size(); i++) {
    unique_lock<mutex> channel_is_recording_lck(this->channel_is_recording_mtx);
    if (i == channel_id)
      this->channel_is_recording[i] = false;
    else 
      this->channel_is_recording[i] = true;
  }
}

void Node::AddMessageToChannelStates(int channel_id, string msg) {
  unique_lock<mutex> channel_states_lck(this->channel_states_mtx);
  this->channel_states[channel_id].push_back(msg);
}

void Node::HandleServerConnection(int conn_fd, string peer_name) {
  string msg;
  struct Message m;

  while (true) {
    printf("Server: total messages: %d\n", this->total_messages);

    msg = ReceiveMessage(conn_fd, 512);
    printf("\nServer %d: received message: %s from %s", conn_fd, msg.c_str(),
           peer_name.c_str());

    m = DeserializeMessage(msg);

    // handling markers and application messages
    if (m.identifier == "M") {
      IncrementMarkersReceived();
      if (IsFirstMarker()) {
        // Set current incoming channel to NULL
        // Start recording other incoming channels
        // Send marker to outgoing neighbours
        StartRecordingChannels(m.value);
        SendMarkerMessages();
      } else {
        // output channel state and stop recording channel
        StopRecordingChannel(m.value);
      }
    } else {                // Application message
      if (this->channel_is_recording[m.value] == true) { // in-transit
        AddMessageToChannelStates(m.value, msg);
      } else { // normal message outside of snapshot
        UpdateTimestamps(m.timestamp);
        DisplayTimestamps("Server");
      }
    }

    if (this->total_messages < this->max_number)
      SetActive();
    else
      SetPassive();
  }
}

void Node::RunServer(int port) {
  Server server(port);

  vector<thread> client_threads;

  struct hostent *he;
  string peer_name;
  while (true) {
    int conn_fd =
        server.Accept(server.listener_fd_, (struct sockaddr *)&server.addr_,
                      &server.addr_size_);

    he = gethostbyaddr(&server.addr_.sin_addr.s_addr,
                       sizeof(server.addr_.sin_addr.s_addr), AF_INET);
    peer_name = split(string(he->h_name), '.')[0];
    client_threads.emplace_back(&Node::HandleServerConnection, this, conn_fd,
                                peer_name);
  }
  for (auto &client_thread : client_threads)
    client_thread.join();
}

int Node::NumberOfMessagesToSend(void) {
  int num_messages = get_random(this->min_per_active, this->max_per_active);
  if (num_messages + this->total_messages > this->max_number)
    num_messages = this->max_number - this->total_messages;
  return num_messages;
}

void Node::HandleClientConnection(int conn_fd) {
  this_thread::sleep_for(chrono::milliseconds(min_send_delay));

  UpdateTimestamps();

  struct Message m = ConstructMessage("A", this->timestamps, this->id);
  string msg = SerializeMessage(m);

  DisplayTimestamps("Client");

  SendMessage(conn_fd, msg, 512);

  printf("Client %d: sent message: %s: length: %ld\n", conn_fd, msg.c_str(),
         msg.length());
}

void Node::RunClient(void) {
  int num_messages;
  int neighbour_conn;

  while (this->total_messages < this->max_number) {
    if (!is_active)
      continue;

    num_messages = NumberOfMessagesToSend();

    for (int i = 0; i < num_messages; i++) {
      {
        unique_lock<mutex> total_messages_lck(this->total_messages_mtx);
        this->total_messages++;
      }
      printf("\nClient: total_messages: %d, num_messages: %d, max_number: %d\n",
            this->total_messages, num_messages, this->max_number);

      neighbour_conn = PickRandomNeighbour().second;
      HandleClientConnection(neighbour_conn);
    }
    SetPassive();
  }

  printf("\nClient done: total_messages: %d, max_number: %d\n",
         this->total_messages, this->max_number);
}

pair<int, int> Node::PickRandomNeighbour(void) {
  int neighbour_id = get_random(0, this->neighbours.size() - 1);
  return this->neighbours[neighbour_id];
}

void Node::SendMarkerMessages(void) {
  struct Message m = ConstructMessage("M", this->timestamps, this->id);
  string msg;
  int conn_fd;

  string file = "config-" + to_string(this->id) + ".txt";
  write_file(file, TimestampToString(this->timestamps) + "\n");

  printf("Node %d: snapshot_state: <%s>\n", this->id,
         TimestampToString(this->timestamps).c_str());

  for (int i = 0; i < this->neighbours.size(); i++) {
    msg = SerializeMessage(m);

    conn_fd = this->neighbours[i].second;
    printf("Node %d sending Marker %s to Neighbour %d\n", this->id, msg.c_str(),
           conn_fd);
    SendMessage(conn_fd, msg, 512);
  }
}

void Node::DisplayChannelStates(void) {
  if (this->channel_states.size() == 0)
    return;

  printf("Channel States:<");
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("Usage: ./node <host> <port> <id>\n");
    exit(1);
  }
  printf("Node: <%s> <%s> <%s>\n", argv[1], argv[2], argv[3]);
  vector<string> lines = read_file("config.txt");

  string host(argv[1]);
  int port = stoi(argv[2]);
  int id = stoi(argv[3]);

  Node *node = new Node(host, port, lines);
  node->SetActive(id);

  node->DisplayStatus();

  thread server_thread(&Node::RunServer, node, node->port);

  this_thread::sleep_for(chrono::seconds(10));

  node->SetNeighbours(lines);
  thread client_thread(&Node::RunClient, node);

  if (node->id == 0) {
    this_thread::sleep_for(chrono::milliseconds(node->snapshot_delay));
    thread snapshot_thread(&Node::SendMarkerMessages, node);
    snapshot_thread.join();
  }

  client_thread.join();
  server_thread.join();

  node->DisplayTimestamps("Final");
  return 0;
}
