#ifndef UTILITY_H_
#define UTILITY_H_

#include <string>
#include <utility>
#include <vector>

std::string trim(const std::string &s);

std::string join(std::vector<std::string> &vec, std::string delimiter);

std::vector<std::string> split(const std::string &s, const char &delimiter);

std::string remove_comment(const std::string &s);

std::vector<std::string> read_file(const std::string &file_name);
void write_file(const std::string &file_name, const std::string &result);

void write_file(const std::string &file_name, const int &timestamp, const int &id,
                const double &cs_execution_time, const std::string &label);

double exponential_distribution(double x);

#endif // UTILITY_H_
