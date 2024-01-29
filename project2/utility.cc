#include "utility.h"

#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <regex>
#include <sstream>
#include <vector>

using namespace std;

std::string trim(const std::string &s) {
  auto start = s.begin();
  while (start != s.end() && std::isspace(*start)) {
    start++;
  }

  auto end = s.end();
  do {
    end--;
  } while (std::distance(start, end) > 0 && std::isspace(*end));

  return std::string(start, end + 1);
}

vector<string> split(const string &s, const char &delimiter) {
  vector<string> result;
  stringstream sstream(s);
  string token;
  while (getline(sstream, token, delimiter)) {
    token = trim(token);
    result.push_back(token);
  }
  return result;
}
string remove_comment(const string &s) {
  string result;
  if (s.find('#') != string::npos) {
    result = split(s, '#')[0];
    return result;
  }
  return s;
}

vector<string> read_file(const string &file_name) {
  ifstream file(file_name);
  if (!file.is_open()) {
    printf("file error");
    exit(1);
  }

  string line = "";
  vector<string> lines;
  while (getline(file, line)) {
    if (line.length() == 0 || line.at(0) == '#')
      continue;
    line = remove_comment(line);
    lines.push_back(line);
  }

  file.close();
  return lines;
}

void write_file(const string &file_name, const string &result) {
  ofstream file(file_name, ios::app);
  if (!file.is_open()) {
    printf("file error");
    exit(1);
  }
  string output = result + "\n";
  file << output;
  file.close();
}

double exponential_distribution(double x) {
  std::random_device dev;
  std::mt19937 rng(dev());
  double lambda = 1 / x;
  std::exponential_distribution<double> distribution(lambda);
  return distribution(rng);
}