#include "Event.h"

bool Event::operator >(const Event& other) const
{
    if (time == other.time) return type > other.type;
    else return time > other.time;
}

create_transaction_object::create_transaction_object(const int creator_node_id)
{
    this->creator_node_id = creator_node_id;
}

receive_transaction_object::receive_transaction_object(const int sender_node_id, const int receiver_node_id,
                                                       const shared_ptr<Transaction>& txn)
{
    this->sender_node_id = sender_node_id;
    this->receiver_node_id = receiver_node_id;
    this->txn = txn;
}

receive_block_object::receive_block_object(const int sender_node_id, const int receiver_node_id,
                                           const shared_ptr<Block>& blk,int tries)
{
    this->sender_node_id = sender_node_id;
    this->receiver_node_id = receiver_node_id;
    this->blk = blk;
    this->tries = tries;
}

block_mined_object::block_mined_object(const int miner_node_id, const shared_ptr<Block>& blk)
{
    this->miner_node_id = miner_node_id;
    this->blk = blk;
}

receive_hash_object::receive_hash_object(long long block_hash,int sender_node_id, int receiver_node_id, const shared_ptr<Block>& blk)
{
    this->sender_node_id= sender_node_id;
    this->receiver_node_id = receiver_node_id;
    this->block_hash = block_hash; // dummy
    this->blk = blk;
}

get_block_request_object::get_block_request_object(int sender_node_id, int receiver_node_id,
    const shared_ptr<Block>& blk)
{
    this->sender_node_id = sender_node_id;
    this->receiver_node_id = receiver_node_id;
    this->blk = blk;
}

timer_expired_object::timer_expired_object(int node_id, const shared_ptr<Block>& blk)
{
    this->node_id = node_id;
    this->blk = blk;
}

release_private_object::release_private_object(int node_id,int counter)
{
    this->node_id = node_id;
    this->counter = counter;
}

ostream& operator<<(ostream& os, const release_private_object& obj)
{
    os << " Release private chain event: "<<endl;
    os << " node_id: " << obj.node_id << endl;
    return os;
}

ostream& operator<<(ostream& os, const timer_expired_object& obj)
{
    os << " Timer expired event: "<<endl;
    os << " Node id: " << obj.node_id << endl;
    return os;
}

ostream& operator<<(ostream& os, const get_block_request_object& obj)
{
    os << "Get block request object: " <<endl;
    os << "Sender: " << obj.sender_node_id << " Receiver: " << obj.receiver_node_id << " Block id: " << obj.blk->id
   << endl;
    return os;
}

ostream& operator<<(ostream& os, const receive_hash_object& obj)
{
    os << "Receive hash object: " <<endl;
    os << "Sender: " << obj.sender_node_id << " Receiver: " << obj.receiver_node_id << " Block id: " << obj.blk->id
    << endl;
    return os;
}

Event::Event(const long long time, const int type, VO object): time(time), type(type), object(std::move(object))
{
}

ostream& operator<<(ostream& os, const create_transaction_object& obj)
{
    os << "Create transaction object: " << endl;
    os << "Creator node id: " << obj.creator_node_id << endl;
    return os;
}

ostream& operator<<(ostream& os, const receive_transaction_object& obj)
{
    os << "Receive transaction object" << endl;
    os << "Sender: " << obj.sender_node_id << " Receiver: " << obj.receiver_node_id << " Transaction id: " << obj.txn->id
        << endl;
    return os;
}

ostream& operator<<(ostream& os, const receive_block_object& obj)
{
    os << "Receive block object" << endl;
    os << "Sender: " << obj.sender_node_id << " Receiver: " << obj.receiver_node_id << " Block id: " << obj.blk->id <<
        endl;
    return os;
}

ostream& operator<<(ostream& os, const block_mined_object& obj)
{
    os << "Block mined object" << endl;
    os << "Miner node id: " << obj.miner_node_id << " Block id: " << obj.blk->id << endl;
    return os;
}

ostream& operator<<(ostream& os, const Event& e)
{
    os << "Event time: " << e.time << endl;
    // std::visit(behaviour , object)
    std::visit([&os](auto& obj) { os << obj << endl; }, e.object);
    return os;
}
