#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <ostream>
#include <set>
#include <memory>
#include <queue>

using namespace std;

extern int number_of_nodes;
extern int percent_malicious_nodes;

class Transaction
{
    // increasing variable to generate unique transaction ids
    static long long transaction_ticket;

public:
    long long id;
    int receiver;
    long long amount;
    bool coinbase;
    int sender;

    Transaction(int receiver, int amount, bool coinbase, int sender = -1);
    friend ostream& operator<<(ostream& os, const Transaction& txn);
};

class Block
{
    // increasing variable to generate unique block ids
    static long long block_ticket;

public:
    long long id;
    shared_ptr<Block> parent_block;
    vector<shared_ptr<Transaction>> transactions;
    long long creation_time;
    bool is_private;
    bool is_honest;

    Block(long long creation_time, shared_ptr<Block> parent_block, bool is_private, bool is_honest);
    friend ostream& operator<<(ostream& os, const Block& block);
};

// Leaf node of Block chain tree
class LeafNode
{
public:
    shared_ptr<Block> block; // last block in that chain
    long long length; // used to determine longest chain
    set<long long> transaction_ids; // to verify if transaction already present in chain
    vector<long long> balance; // balance of each peer in that chain for easy validation of transactions

    LeafNode(shared_ptr<Block> block, long long length);
    friend ostream& operator<<(ostream& os, const LeafNode& leaf);

};

class Timer {
    public:
    shared_ptr<Block> blk;
    queue<int> available_senders;
    set<int> tried_senders;
    bool is_running;

    Timer(shared_ptr<Block> blk, bool is_runninng);
};

// comparator to sort  LeafNode pointers by length of leaf node
struct CompareLeafNodePtr {
    bool operator()(const std::shared_ptr<LeafNode>& a, const std::shared_ptr<LeafNode>& b) const;
};

#endif //BLOCKCHAIN_H
