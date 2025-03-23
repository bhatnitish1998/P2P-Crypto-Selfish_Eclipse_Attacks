#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <iostream>
#include "Network.h"
#include <fstream>
#include "utility_functions.h"
#include <filesystem>

using namespace std;

namespace fs = filesystem;

extern int initial_bitcoin;
extern int initial_number_of_transactions;
extern int mean_transaction_inter_arrival_time;

class Network;
extern Network network;

class Simulator
{
    // Initializes each nodes blockchain with genesis block containing starting balances of all nodes
    void create_genesis();

    // Populates event queue with CREATE_TRANSACTION events
    static void create_initial_transactions();

public:
    Network network;
    // Assign initial balance, create genesis block, create initial transactions
    void initialize();
    // Start processing event queue
    void start();
    // write stats of each node to file
    void write_node_stats_to_file();
    // creates a csv files to store all nodes details
    void write_all_node_details_to_file(const vector<Node>& nodes, const string &fname);
};

struct block_stats
{
    long long block_id;
    long long parent_block_id;
    long long first_seen_time;
    long long num_transactions;
    bool part_of_longest;
    bool generated_by_node;

    block_stats();
    bool operator <(const block_stats & other) const;
};



#endif //SIMULATOR_H
