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
extern int propagation_delay_min;
extern int propagation_delay_max;;
extern long long simulation_time;
extern double total_hashing_power;
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
protected:
  // to generate unique node ids (equal to index in node vector)
  static int node_ticket;

public:
  int id;
  bool fast;
  bool high_cpu;
  double hashing_power;
  bool currently_mining;
  vector<Link> peers; // stores links to all its peers
  shared_ptr<Block> genesis; // genesis block pointer
  set<shared_ptr<LeafNode>,CompareLeafNodePtr> leaves; // stores information about all leaf nodes of blockchain tree
  queue<shared_ptr<Transaction>> mempool;
  set <long long> transactions_in_pool;
  map<long long, long long> block_ids_in_tree; // stores received blocks <block id, time first seen>
  long long transactions_received;
  long long blocks_received;

  Node();
  virtual ~Node() {};
  // Function to compute the union of peers and malicious_peers
  virtual size_t get_union_of_peers_size(); 

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



// Malicious Node 
class MaliciousNode : public Node {
public:
  vector<Link> malicious_peers; 
  MaliciousNode();
  size_t get_union_of_peers_size();
};

// Ringmaster
class RingMasterNode : public MaliciousNode {
public:
  int global_chain_length;
  int private_chain_length;
  RingMasterNode();

};



class Network
{
public:
  vector<shared_ptr<Node>> nodes;
  vector<int> malicious_node_ids; // indexes of subset of nodes which are only malicious
  vector<int> honest_node_ids;
  int ringmaster_node_id;

  Network();
  
  void build_network(vector<int> &node_ids,string networkType); 
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
