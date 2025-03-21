#include "Network.h"

#include <algorithm>
#include <iostream>

#include "utility_functions.h"

int Node::node_ticket = 0;

Link::Link(const int peer, const int propagation_delay, const long long link_speed)
{
    this->peer = peer;
    this->propagation_delay = propagation_delay;
    this->link_speed = link_speed;
}

Node::Node()
{
    id = node_ticket++;
    fast = false;
    high_cpu = false;
    peers.reserve(6);
    genesis = nullptr;
    currently_mining = false;
    transactions_received = 0;
    blocks_received = 0;
}

MaliciousNode::MaliciousNode() : Node() {
    malicious_peers.reserve(6);
    fast = true;
}

RingMasterNode::RingMasterNode() : MaliciousNode() {
}

// Function to compute the size of union of peers and malicious_peers
size_t Node::get_union_of_peers_size() {
    return peers.size();
}

size_t MaliciousNode::get_union_of_peers_size() {
    std::set<int> union_set;

    // Add all peer indices from peers
    for (const Link& link : peers) {
        union_set.insert(link.peer);
    }

    // Add all peer indices from malicious_peers
    for (const Link& link : malicious_peers) {
        union_set.insert(link.peer);
    }

    return union_set.size();
}

void Node::create_transaction()
{
    // randomly choose amount and receiver
    const int receiver = uniform_distribution(0, number_of_nodes - 1);
    const int amount = uniform_distribution(transaction_amount_min, transaction_amount_max);

    const auto t = make_shared<Transaction>(receiver,amount,false,id);
    mempool.push(t);
    transactions_in_pool.insert(t->id);

    l.log<<"Time "<< simulation_time << ": Node "<<id<<" created Transaction " << *t <<endl;
    // send to all peers
    for (Link& x: peers)
        send_transaction_to_link(t,x);

    // if free start mining
    if ( !currently_mining) mine_block();
}


void Node::send_transaction_to_link(const shared_ptr<Transaction>& txn, Link& link) const
{
    // Compute link latency
    const long long latency = link.propagation_delay + transaction_size/link.link_speed + \
    exponential_distribution(static_cast<double>(queuing_delay_constant)/static_cast<double>(link.link_speed));

    // send transaction by creating receive transaction event for recipient
    link.transactions_sent.insert(txn->id);
    receive_transaction_object obj(id,link.peer,txn);
    event_queue.emplace(simulation_time + latency,RECEIVE_TRANSACTION,obj);
}

void Node::receive_transaction(const receive_transaction_object& obj)
{
    transactions_received++;
    // add transaction to the mempool if not present
    if (transactions_in_pool.count(obj.txn->id)==0)
    {
        transactions_in_pool.insert(obj.txn->id);
        mempool.push(obj.txn);
        l.log << "Time "<< simulation_time <<": Node " << id << " received transaction "<<obj.txn->id<<" from " << obj.sender_node_id<<endl;
    }

    // if free start mining
    if (!currently_mining) mine_block();

    // send it to the first unsent peer
    for (Link& x : peers)
    {
        if (x.peer != obj.sender_node_id && x.transactions_sent.count(obj.txn->id) == 0)
        {
            send_transaction_to_link(obj.txn, x);
            return;
        }
    }
}

void Node::receive_block(const receive_block_object& obj)
{
    // check if block already present
    if (block_ids_in_tree.count(obj.blk->id) == 1)
        return;

    blocks_received++;
    l.log<< "Time "<< simulation_time <<": Node " << id << " received block "<<obj.blk->id<<" from "<<obj.sender_node_id<<endl;

    // if block received before parent
    if (block_ids_in_tree.count(obj.blk->parent_block->id) == 0)
    {
        l.log <<"Time "<< simulation_time << ": Node " << id << " NACK block  "<<obj.blk->id<<endl;
        // simulate resending the block by the peer (assume ACK, NACK mechanism)
        for (const auto& link: peers)
        {
            if (link.peer == obj.sender_node_id)
            {
                const long long size = transaction_size * static_cast<long long>(obj.blk->transactions.size());
                const long long latency = link.propagation_delay + size/link.link_speed + \
                exponential_distribution(static_cast<double>(queuing_delay_constant)/static_cast<double>(link.link_speed));
                Event e(simulation_time + latency,RECEIVE_BLOCK,receive_block_object(obj.sender_node_id,obj.receiver_node_id,obj.blk));
                event_queue.push(e);
            }
        }
        return;
    }
    // if validated and added to the longest chain, re-start mining on longest chain
    if (validate_and_add_block(obj.blk))
    {
        l.log << "Time "<< simulation_time <<": Node " << id << " block  "<<obj.blk->id<< " extended longest chain" << endl;
        mine_block();
    }
}

bool Node::validate_and_add_block(shared_ptr<Block> blk)
{
    // check if the parent node of the block is leaf node
    const auto it = find_if(leaves.begin(),leaves.end(),\
        [&blk](const shared_ptr<LeafNode>& leaf){return blk->parent_block->id == leaf->block->id;});
    vector<long long > temp_balance;
    set<long long > temp_transaction_ids;
    long long temp_length =1;

    // if parent not a leaf
    if (it == leaves.end())
    {
        temp_balance.resize(number_of_nodes,0);

        // traverse the chain till genesis and get balance and transaction ids
        auto temp_ptr = blk->parent_block;
        while (temp_ptr)
        {
            for (const auto& txn: temp_ptr->transactions)
            {
                temp_transaction_ids.insert(txn->id);

                if (txn->coinbase) temp_balance[txn->receiver]+=txn->amount;
                else
                {
                    temp_balance[txn->sender]-=txn->amount;
                    temp_balance[txn->receiver]+=txn->amount;
                }
            }
            temp_ptr = temp_ptr->parent_block;
            temp_length++;
        }
    }
    // if parent is a leaf get it from leaf node
    else
    {
        temp_balance = (*it)->balance;
        temp_transaction_ids = (*it)->transaction_ids;
        temp_length = (*it)->length+1;
    }


    // validate the block
    for (const auto& txn: blk->transactions)
    {
        temp_transaction_ids.insert(txn->id);
        if (txn->coinbase) temp_balance[txn->receiver]+=txn->amount;
        else
        {
            temp_balance[txn->sender]-= txn->amount;
            // if balance -ve invalid transaction, abort
            if ( temp_balance[txn->sender] < 0 )
            {
                l.log << "Time "<< simulation_time <<": Node " << id << " validation fail block  "<<blk->id<<endl;
                return false;
            }
            temp_balance[txn->receiver]+= txn->amount;
        }
    }

    // if validated broadcast block and insert into tree.
    broadcast_block(blk);
    block_ids_in_tree.insert({blk->id,simulation_time});

    l.log << "Time "<< simulation_time <<": Node " << id << " successfully validated block  "<<blk->id<<endl;

    // Update the leaf nodes
    const auto temp_leaf = make_shared<LeafNode>(blk,temp_length);
    temp_leaf->balance= std::move(temp_balance);
    temp_leaf->transaction_ids = std::move(temp_transaction_ids);

    // return true if longest changes after inserting
    const long long previous_longest = (*leaves.begin())->block->id;
    if (it != leaves.end())
        leaves.erase(it);
    leaves.insert(temp_leaf);
    const long long current_longest = (*leaves.begin())->block->id;

    return (previous_longest != current_longest);
}


void Node::mine_block()
{
    currently_mining = true;
    if (mempool.empty())
    {
        currently_mining = false;
        return;
    }
    // create the new block with coinbase transaction
    shared_ptr<LeafNode> longest_leaf = *leaves.begin();
    auto blk = make_shared<Block>(simulation_time,longest_leaf->block);
    blk->transactions.push_back(make_shared<Transaction>(id,mining_reward,true));
    vector<long long > temp_balance = longest_leaf->balance;

    // populate block with valid transactions from mempool
    blk->transactions.reserve(min(static_cast<int>(mempool.size()),1000));
    while (blk->transactions.size()< 1000 && !mempool.empty())
    {
        auto txn = mempool.front(); mempool.pop();
        transactions_in_pool.erase(txn->id);

        if (longest_leaf->transaction_ids.count(txn->id) == 0)
        {
            if (txn->coinbase) temp_balance[txn->receiver]+=txn->amount;
            else
            {
                if ( temp_balance[txn->sender] - txn->amount < 0 )
                    continue;

                temp_balance[txn->sender]-= txn->amount;
                temp_balance[txn->receiver]+= txn->amount;
            }
        blk->transactions.push_back(txn);
        }
    }
    if (blk->transactions.size() <=1)
    {
            currently_mining = false;
            return;
    }

    l.log << "Time " << simulation_time << ": Node " << id << " started mining "<<blk->id<<endl;
    // compute mining time and create event at that time
    const double hashing_fraction = (high_cpu? 10.0 : 1.0)/static_cast<double>(total_hashing_power);
    const long long mining_time = exponential_distribution(block_inter_arrival_time/hashing_fraction);
    block_mined_object obj(id,blk);
    event_queue.emplace(simulation_time + mining_time,BLOCK_MINED, obj);
}


void Node::complete_mining(const shared_ptr<Block>&  blk)
{
    shared_ptr<LeafNode> longest_leaf = *leaves.begin();
    // if longest chain has not changed while mining
    if (blk->parent_block->id == longest_leaf->block->id)
    {
        // validation always succeeds
        validate_and_add_block(blk);
        l.log << "Time " << simulation_time << ": Node " << id << " successfully mined "<<blk->id<<endl;
        // start mining next block
        mine_block();
    }

    // if failed return transactions to the mempool
    else
    {
        l.log << "Time " << simulation_time << ": Node " << id << " mining event ignored "<<blk->id<<endl;
        for (const auto& txn: blk->transactions)
        {
            if (transactions_in_pool.count(txn->id) == 0 && !txn->coinbase)
            {
                mempool.push(txn);
                transactions_in_pool.insert(txn->id);
            }
        }
        mine_block();
    }
}

void Node::broadcast_block(const shared_ptr<Block>& blk)
{
    const long long size = (transaction_size) * static_cast<long long>(blk->transactions.size());

    for (auto link : peers)
    {
        // send block if not already sent to the peer
        if (link.blocks_sent.count(blk->id) == 0)
        {
            link.blocks_sent.insert(blk->id);
            const long long latency = link.propagation_delay + size/link.link_speed + \
            exponential_distribution(static_cast<double>(queuing_delay_constant)/static_cast<double>(link.link_speed));

            // create receive block event for that node at current time + latency
            receive_block_object obj(id,link.peer,blk);
            Event e(simulation_time + latency,RECEIVE_BLOCK,obj);
            event_queue.push(e);
        }
    }
}

void Network::build_network(vector<int> &node_ids,string networkType){
    int done = false;
    map<int, vector<int>> mal;  // Adjacency list as a map

    sort(node_ids.begin(), node_ids.end());

    for (int node_idx : node_ids) {
        mal[node_idx] = {};
    }


    // Until graph is connected
    while (!done) {
        for (int i = 0; i < node_ids.size(); i++) {
            int node_idx = node_ids[i];
            int min_peers = min(3, static_cast<int>(node_ids.size() - 1));

            // Until at least connected to min peers, keep adding neighbors
            while (mal[node_idx].size() < min_peers) {

                // Create excluded list
                vector<int> excluded;
                excluded.push_back(node_idx);
                for (int j = 0; j < node_ids.size(); j++) {
                    if (find(mal[node_idx].begin(), mal[node_idx].end(), node_ids[j]) != mal[node_idx].end()) {
                        excluded.push_back(node_ids[j]);
                    }
                }

                // Choose new neighbors
                vector<int> temp = choose_neighbours_values(
                    node_ids,
                    min_peers - static_cast<int>(mal[node_idx].size()),
                    excluded
                );


                for (auto neighbour : temp) {
                    int nei_idx = neighbour;

                    // Ensure max 6 peers, no self-connection, and no duplicate edges
                    if (nei_idx != node_idx &&
                        mal[nei_idx].size() < min(6, static_cast<int>(node_ids.size() - 1)) &&
                        find(mal[node_idx].begin(), mal[node_idx].end(), nei_idx) == mal[node_idx].end()) {
                        mal[node_idx].push_back(nei_idx);
                        mal[nei_idx].push_back(node_idx);
                    }
                }

            }
        }
        
        done = check_connected_map(mal);  // Check if the graph is connected
        
        // Store network to file if done
        if (done) {
            string fname = "network_" + networkType + ".txt";
            write_network_to_file_map(mal, fname);
        }
        
    }

    // set up link speed and propagation delay for each peer
    for (int i: node_ids)
    {
        for (auto x : mal[i])
        {
            if (i < x)
            {
                int propagation_delay = uniform_distribution(propagation_delay_min, propagation_delay_max); // 1ms to 10ms
                int link_speed = nodes[i]->fast && nodes[x]->fast ? 100 * 1000 : 5 * 1000; // bits per millisecond
                
                if (networkType == "common"){
                    nodes[i]->peers.emplace_back(x, propagation_delay, link_speed);
                    nodes[x]->peers.emplace_back(i, propagation_delay, link_speed);
                }
                else{
                    MaliciousNode* maliciousNodeI = dynamic_cast<MaliciousNode*>(nodes[i].get());
                    MaliciousNode* maliciousNodeX = dynamic_cast<MaliciousNode*>(nodes[x].get());
                    maliciousNodeI->malicious_peers.emplace_back(x, propagation_delay, link_speed);
                    maliciousNodeX->malicious_peers.emplace_back(i, propagation_delay, link_speed);
                }
            }
        }
    }

    

}


Network::Network()
{
    // Node id equal to its index in vector
    nodes.resize(number_of_nodes);
    vector<int> all_node_ids;
    for (int i=0; i<number_of_nodes; i++){
        all_node_ids.push_back(i);
    }

    //   Make ( n * percent_fast) fast nodes
    malicious_node_ids = choose_percent(number_of_nodes, percent_malicious_nodes / 100.0);
    bool assigned_ringmaster = false;
    for(int i=0; i<number_of_nodes; i++){
        if (find(malicious_node_ids.begin(), malicious_node_ids.end(), i) != malicious_node_ids.end()){
            if (assigned_ringmaster) 
                nodes[i] = make_shared<MaliciousNode>();
            else{
                nodes[i] = make_shared<RingMasterNode>();
                assigned_ringmaster = true;
                ringmaster_node_id = nodes[i]->id;
                cout<<"Ringmaster id: " << ringmaster_node_id;
            }
        }
        else{
            honest_node_ids.push_back(i);
            nodes[i] = make_shared<Node>();   // honest_node created
        }
    }

    cout<< "Malicious node ids: ";
    for(int i: malicious_node_ids){
        cout<< " "<< i; 
    }


    cout<< "\nHonest node ids: ";
    for(int i: honest_node_ids){
        cout<< " "<< i; 
    }

    total_hashing_power = number_of_nodes; // all nodes have same hashing power; consider unit of hashing power per node = 1.
    total_hashing_power_malicious = malicious_node_ids.size() * 1;

    build_network(all_node_ids, "common");
    build_network(malicious_node_ids, "malicious");


    write_node_details_to_file(nodes, "all_node_details.csv");

    // bool done = false;
    // vector<vector<int>> al(number_of_nodes);

    // // Until graph is connected
    // while (!done)
    // {
    //     al.clear();
    //     al.resize(number_of_nodes);

    //     for (int i = 0; i < number_of_nodes; i++)
    //     {
    //         int min_peers = min(3,number_of_nodes-1);
    //         // until at least connected to min peers keep adding neighbors (reverse addition leads to > 3 but < 6 links)
    //         while (al[i].size() < min_peers)
    //         {
    //             vector<int> temp = choose_neighbours_values(honest_node_ids, min_peers - static_cast<int>(al[i].size()), al[i]);
    //             for (auto neighbour : temp)
    //             {
    //                 // ensure max 6 peers
    //                 if (neighbour != i && al[neighbour].size() < min(6,number_of_nodes-1))
    //                 {
    //                     al[i].push_back(neighbour);
    //                     al[neighbour].push_back(i);
    //                 }
    //             }
    //         }
    //     }
    //     done = check_connected(al);

    //     // store network to file if done
    //     if(done) write_network_to_file(al,"network.txt");
    // }

    // // set up link speed and propagation delay for each peer
    // for (int i = 0; i < number_of_nodes; i++)
    // {
    //     for (auto x : al[i])
    //     {
    //         if (i < x)
    //         {
    //             int propagation_delay = uniform_distribution(propagation_delay_min, propagation_delay_max);
    //             int link_speed = nodes[i]->fast && nodes[x]->fast ? 100 * 1000 : 5 * 1000; // bits per millisecond
    //             nodes[i]->peers.emplace_back(x, propagation_delay, link_speed);
    //             nodes[x]->peers.emplace_back(i, propagation_delay, link_speed);
    //         }
    //     }
    // }


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    // setup similar network for malicious_nodes

    


}

Logger::Logger()
{
    if (fs::path dir = "Output/Log"; !fs::exists(dir))
        fs::create_directories(dir);
    log.open("Output/Log/log.txt");
}

Logger::~Logger()
{
    log.close();
}