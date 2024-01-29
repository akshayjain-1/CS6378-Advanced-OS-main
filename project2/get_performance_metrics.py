#!/usr/bin/env python3

import csv
import sys

def read_csv(file_name):
    rows = []
    with open(file_name) as f:
        csvreader = csv.reader(f)
        for row in csvreader:
            rows.append(row)
    return rows

def read_log(file_name):
    with open(file_name) as f:
        csvreader = csv.reader(f)
        line_count = 1
        for row in csvreader:
            node_id, top_request_timestamp, top_request_id = row[0], row[1], row[2]
            if node_id != top_request_id:
                print(f"{file_name}: error at line {line_count}: {row}")
                return
            line_count += 1

    print(f"{file_name}: No error")

def response_time_vs_request_delay(file_name):
    total_response_time = 0
    total_request_delay = 0
    count = 0
    with open(file_name) as f:
        csvreader = csv.reader(f)
        for row in csvreader:
            count += 1
            response_time, request_delay = row[0], row[1]
            total_response_time += float(response_time)
            total_request_delay += float(request_delay)
    return total_response_time / count, total_request_delay / count

def system_throughput_vs_request_delay(file_name):
    total_system_throughput = 0
    total_request_delay = 0
    count = 0
    with open(file_name) as f:
        csvreader = csv.reader(f)
        for row in csvreader:
            count += 1
            system_throughput, request_delay = row[0], row[1]
            total_system_throughput += float(system_throughput)
            total_request_delay += float(request_delay)
    return total_system_throughput / count, total_request_delay / count
    # print(f"Mean System Throuhgput: {total_system_throughput / count}")
    # print(f"Mean Request Delay: {total_request_delay / count}")

if __name__ == "__main__":
    if (len(sys.argv) != 2):
        print("Usage: ./test.py <num_nodes>")
        sys.exit(1)
    num_nodes = int(sys.argv[1])
    
    total_response_time = 0
    total_request_delay1 = 0

    total_system_throughput = 0
    total_request_delay2 = 0

    for i in range(num_nodes):
        file0_name = f"output{i}_log.csv"
        file1 = f"response_time{i}.csv"
        file2 = f"system_throughput{i}.csv"
        response_time, request_delay1 = response_time_vs_request_delay(file1)
        total_response_time += response_time
        total_request_delay1 += request_delay1

        system_throughput, request_delay2 = system_throughput_vs_request_delay(file2)
        total_system_throughput += system_throughput
        total_request_delay2 += request_delay2

    print(f"Mean Response Time: {total_response_time / num_nodes} vs Mean Inter Request Delay: {total_request_delay1 / num_nodes}")
    print(f"Mean System Throughput: {total_system_throughput / num_nodes} vs Mean Inter Request Delay: {total_request_delay2 / num_nodes}")