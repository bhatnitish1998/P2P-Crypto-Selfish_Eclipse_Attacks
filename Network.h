#ifndef NETWORK_H
#define NETWORK_H

#include <vector>
#include <set>
#include <map>
#include <queue>
#include "Blockchain.h"
#include "Event.h"
#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = filesystem;

extern int transaction_amount_min;
extern int transaction_amount_max;
extern int queuing_delay_constant;
extern int percent_fast;
extern int percent_high_cpu;
extern int percent_fast_malicious;
extern int percent_high_cpu_malicious;
extern long long total_hashing_power_malicious;
extern int propagation_delay_min;
extern int propagation_delay_max;;
extern long long simulation_time;
extern long long total_hashing_power;
extern int block_inter_arrival_time;
extern int transaction_size;
extern int mining_reward;
extern EQ event_queue;


// Link between two nodes
class Link
{
public:
  int peer; // peer node id
  int propagation_delay;
  long long link_speed;

  // keeps track of transactions and blocks sent to avoid loops
  set<long long> transactions_sent;
  set<long long> blocks_sent;

  Link(int peer, int propagation_delay, long long link_speed);
};

class Node
{
private:
  // to generate unique node ids (equal to index in node vector)
  static int node_ticket;

public:
  int id;
  bool fast;
  bool high_cpu;
  bool currently_mining;
  bool malicious;
  bool ringmaster;
  vector<Link> malicious_peers; // stores links to all its malicious peers in the attacker-only-overlay-netwrok
  vector<Link> peers; // stores links to all its peers
  shared_ptr<Block> genesis; // genesis block pointer
  set<shared_ptr<LeafNode>,CompareLeafNodePtr> leaves; // stores information about all leaf nodes of blockchain tree
  queue<shared_ptr<Transaction>> mempool;
  set <long long> transactions_in_pool;
  map<long long, long long> block_ids_in_tree; // stores received blocks <block id, time first seen>
  long long transactions_received;
  long long blocks_received;

  Node();

  // Function to compute the union of peers and malicious_peers
  size_t get_union_of_peers_size(); 

  // creates a random transaction and broadcasts it to its peers
  void create_transaction();

  //  receive a transaction from peer
  void receive_transaction(const receive_transaction_object &obj);

  // send transaction to particular link
  void send_transaction_to_link(const shared_ptr<Transaction>& txn, Link &link) const;

  // receive a block from peer
  void receive_block(const receive_block_object &obj);

  // true: block added to the longest chain
  // false: validation failed or block added to some forked branch
  bool validate_and_add_block(shared_ptr<Block> blk);

  // Prepare block and start mining
  void mine_block();

  // add mined block to tree if longest not changed
  void complete_mining(const shared_ptr<Block>& blk);

  // broadcast block to all peers
  void broadcast_block(const shared_ptr<Block>& blk);
};

class Network
{
public:
  vector<Node> nodes;
  vector<int> malicious_node_ids; // indexes of subset of nodes which are only malicious

  Network();
};

class Logger
{
public:
  ofstream log;

  Logger();
  ~Logger();
};

extern Logger l;

#endif //NETWORK_H
