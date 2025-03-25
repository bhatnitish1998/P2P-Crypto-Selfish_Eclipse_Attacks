#include "Blockchain.h"

#include <utility>

long long Transaction::transaction_ticket = 0;
long long Block::block_ticket = 0;

Transaction::Transaction(const int receiver, const int amount, const bool coinbase, const int sender)
{
    id = transaction_ticket++;
    this->receiver = receiver;
    this->amount = amount;
    this->coinbase = coinbase;
    this->sender = sender;

    if (coinbase && sender != -1) throw invalid_argument("sender present for coinbase transaction");
}

Block::Block(const long long creation_time,  shared_ptr<Block> parent_block,bool is_honest)
{
    id = block_ticket++;
    this->parent_block = std::move(parent_block);
    this->creation_time = creation_time;
    this->is_honest = is_honest;
}

LeafNode::LeafNode( shared_ptr<Block> block, const long long length)
{
    this->block = std::move(block);
    this->length = length;
    this->balance.resize(number_of_nodes, 0);
}

bool CompareLeafNodePtr::operator()(const std::shared_ptr<LeafNode>& a, const std::shared_ptr<LeafNode>& b) const
{
    if (a->length == b->length) return a->block->id < b->block->id;
    else return a->length > b->length;
}

Timer::Timer(shared_ptr<Block> blk)
{
    this->blk = std::move(blk);
}

ostream& operator<<(ostream& os, const Transaction& txn)
{
    if (txn.coinbase)
        os << txn.id << ": " << txn.receiver << " mines " << txn.amount << " coins";
    else
        os << txn.id << ": " << txn.sender << " pays " << txn.receiver << " " << txn.amount << " coins";
    return os;
}

ostream& operator<<(ostream& os, const Block& block)
{
    os << "Block id: " << block.id << " Txns: " << block.transactions.size() << endl;
    for (const shared_ptr<Transaction>& txn : block.transactions)
        os << *txn << endl;
    return os;
}

ostream& operator<<(ostream& os, const LeafNode& leaf)
{
    os << "Leaf length: " << leaf.length << endl;
    os << "Block: " << *leaf.block;
    os << "Transactions: ";
    for (const auto x : leaf.transaction_ids)
        os << x << "\t";
    os << endl;
    os << "Balance: ";
    for (const auto x : leaf.balance)
        os << x << "\t";
    os << endl;
    return os;
}
