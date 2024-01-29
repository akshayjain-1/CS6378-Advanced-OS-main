#include "message.h"

#include <vector>

#include "utility.h"

struct Message ConstructMessage(std::string identifier, int timestamp, int id) {
  struct Message m;
  m.identifier = identifier;
  m.timestamp = timestamp;
  m.id = id;

  return m;
}

std::string SerializeMessage(struct Message m) {
  std::string msg = "";
  msg += m.identifier + "|";
  msg += std::to_string(m.timestamp) + "|";
  msg += std::to_string(m.id);

  return msg;
}

struct Message DeserializeMessage(std::string msg) {
  struct Message m;
  std::vector<std::string> result = split(msg, '|');
  m.identifier = result[0];
  m.timestamp = stoi(result[1]);
  m.id = stoi(result[2]);

  return m;
}