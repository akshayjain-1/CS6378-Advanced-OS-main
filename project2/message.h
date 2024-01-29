#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <string>

// [Request/Grant/Release | <vector timestamp T> | ID]
struct Message {
  std::string identifier;
  int timestamp;
  int id;
};

struct Message ConstructMessage(std::string identifier,
                                int timestamp, int value);

std::string SerializeMessage(struct Message m);

struct Message DeserializeMessage(std::string msg);

#endif // MESSAGE_H_