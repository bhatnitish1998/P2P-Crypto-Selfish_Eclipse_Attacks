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
#include <algorithm>
#include <iostream>
#include "utility_functions.h"




typedef variant<shared_ptr<Transaction>, shared_ptr<Block>> MO;

using namespace std;
namespace fs = filesystem;

extern int transaction_amount_min;
extern int transaction_amount_max;
extern int queuing_delay_constant;
extern int propagation_delay_min;
extern int propagation_delay_max;;
extern int propagation_delay_malicious_min;
extern int propagation_delay_malicious_max;
extern long long simulation_time;
extern int block_inter_arrival_time;
extern int timer_timeout_time;
extern int transaction_size;
extern int hash_size;
extern int get_message_size;
extern int mining_reward;
extern EQ event_queue;
extern bool eclipse_attack;
extern bool selfish_mining;
extern int maximum_retries;
extern int global_send_private_counter;
extern string output_dir;
extern bool mitigation;

// Link between two nodes
class Link
{
public:
  int peer; // peer node id
  int propagation_delay;
  long long link_speed;
  long long failed;

  // keeps track of transactions and blocks sent to avoid loops
  set<long long> transactions_sent;
  set<long long> blocks_sent;
  set<long long> get_message_sent;
  set<long long> hash_sent;
  set<int> release_private_sent;

  Link(int peer, int propagation_delay, long long link_speed);
};

class Node
{
private:
  // to generate unique node ids (equal to index in node vector)
  static int node_ticket;

public:
  // Node attributes
  int id;
  bool fast;
  bool malicious;
  bool ringmaster;
  bool currently_mining;
  queue<shared_ptr<Transaction>> mempool;
  set <long long> transactions_in_pool;
  long long hashing_power{};

  // Links
  vector<Link> peers; // stores links to all its peers
  vector<Link> malicious_peers; // empty for honest

  // Blockchain
  shared_ptr<Block> genesis; // genesis block pointer
  set<shared_ptr<LeafNode>,CompareLeafNodePtr> leaves; // stores information about all leaf nodes of blockchain tree
  map<long long, long long> block_ids_in_tree; // stores received blocks <block id, time first seen>
  shared_ptr<LeafNode> private_leaf; // for ringmaster

  // Statistics
  long long transactions_received;
  long long blocks_received;

  // Timers
  map <long long, Timer> timers; // block id and timer object
  set <long long> hashes_seen; // stores block id

  Node();
  // creates a random transaction and broadcasts it to its peers
  void create_transaction();
  //  receive a transaction from peer
  void receive_transaction(const receive_transaction_object &obj);
  // send transaction and get requests to particular link
  void send_transaction_to_link(const shared_ptr<Transaction>& txn, Link &link) const;
  void send_get_to_link(const shared_ptr<Block>& blk, Link &link) const;
  // receive hash from peer
  void receive_hash(const receive_hash_object& obj);
  void timer_expired(const timer_expired_object &obj);
  // Prepare block and start mining
  void mine_block();
  // add mined block to tree if longest not changed
  void complete_mining(const shared_ptr<Block>& blk);
  // true: block added to the longest chain
  // false: validation failed or block added to some forked branch
  bool validate_and_add_block(shared_ptr<Block> blk);
  void broadcast_hash(const shared_ptr<Block>& blk);
  // receive a block from peer
  void receive_block(const receive_block_object &obj);
  // send block to requester
  void send_block(const get_block_request_object &obj);
  long long compute_hash(shared_ptr<Block> blk);
  void release_private(int counter);
  void release_private_helper(shared_ptr<Block> blk);
};



class Network
{
private:
  Network();

public:
  vector<Node> nodes;
  vector<int> malicious_node_ids; // indexes of subset of nodes which are only malicious
  vector<int> honest_node_ids;
  int ringmaster_node_id;


  // Static method to access the singleton instance.
  static Network& getInstance();
  // Delete copy constructor and assignment operator to prevent copies.
  Network(const Network&) = delete;
  Network& operator=(const Network&) = delete;

  void build_network(vector<int> &node_ids,const string& networkType);
};

class Logger
{
public:
  ofstream log;
  string output_dir;

  Logger();
  ~Logger();

  void setOutputDir(const std::string& dir); 
};

extern Logger l;

#endif //NETWORK_H
