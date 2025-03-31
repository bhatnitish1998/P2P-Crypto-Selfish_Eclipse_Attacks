#ifndef EVENT_H
#define EVENT_H

/*
 * List of events
 * Note: SEND_BLOCK and MINE_BLOCK are implicitly carried out. The first when a node completes PoW and the latter when
 * the longest chain changes or when it has transactions left in its mempool to create a block.
 */
#define CREATE_TRANSACTION 0
#define RECEIVE_TRANSACTION 1
#define RECEIVE_BLOCK 2
#define BLOCK_MINED 3
#define RECEIVE_HASH 4
#define GET_BLOCK_REQUEST 5
#define TIMER_EXPIRED 6
#define RELEASE_PRIVATE 7

#include <variant>
#include <queue>
#include "Blockchain.h"

// structures for each event carrying required information
struct create_transaction_object
{
    int creator_node_id;
    explicit create_transaction_object(int creator_node_id);
    friend ostream& operator<<(ostream& os, const create_transaction_object& obj);
};

struct receive_transaction_object
{
    int sender_node_id;
    int receiver_node_id;
    shared_ptr<Transaction> txn;

    receive_transaction_object(int sender_node_id, int receiver_node_id, const shared_ptr<Transaction>& txn);
    friend ostream& operator<<(ostream& os, const receive_transaction_object& obj);
};

struct receive_block_object
{
    int sender_node_id;
    int receiver_node_id;
    shared_ptr<Block> blk;
    receive_block_object(int sender_node_id, int receiver_node_id, const shared_ptr<Block>& blk);
    friend ostream& operator<<(ostream& os, const receive_block_object& obj);
};

struct block_mined_object
{
    int miner_node_id;
    shared_ptr<Block> blk;
    explicit block_mined_object(int miner_node_id, const shared_ptr<Block>& blk);
    friend ostream& operator<<(ostream& os, const block_mined_object& obj);
};

struct receive_hash_object
{
    int sender_node_id;
    int receiver_node_id;
    long long block_hash;
    shared_ptr<Block> blk;
    receive_hash_object(long long block_hash,int sender_node_id, int receiver_node_id, const shared_ptr<Block>& blk);
    friend ostream& operator<<(ostream& os, const receive_hash_object& obj);

};

struct get_block_request_object
{
    int sender_node_id;
    int receiver_node_id;
    shared_ptr<Block> blk;
    get_block_request_object(int sender_node_id, int receiver_node_id, const shared_ptr<Block>& blk);
    friend ostream& operator<<(ostream& os, const get_block_request_object& obj);

};

struct timer_expired_object
{
    int node_id;
    shared_ptr<Block> blk;

    timer_expired_object(int node_id, const shared_ptr<Block>& blk);
    friend ostream& operator<<(ostream& os, const timer_expired_object& obj);
};

struct release_private_object
{
    int node_id;
    int counter;

    release_private_object(int node_id,int counter);
    friend ostream& operator<<(ostream& os, const release_private_object& obj);
};


typedef variant<create_transaction_object, receive_transaction_object, receive_block_object, block_mined_object, receive_hash_object
,get_block_request_object,timer_expired_object, release_private_object> VO;

class Event
{
public:
    long long time;
    int type;
    VO object; // variant holding respective event object

    Event(long long time, int type, VO object);

    // to sort based on time
    bool operator >(const Event& other) const;
    friend ostream& operator<<(ostream& os, const Event& e);
};

typedef priority_queue<Event, vector<Event>, greater<>> EQ;

#endif //EVENT_H
