#include "Network.h"

#include "Simulator.h"

int Node::node_ticket = 0;

Link::Link(const int peer, const int propagation_delay, const long long link_speed)
{
    this->peer = peer;
    this->propagation_delay = propagation_delay;
    this->link_speed = link_speed;
    this->failed =0;
}

Node::Node()
{
    id = node_ticket++;
    fast = false;
    malicious = false;
    ringmaster = false;
    currently_mining = false;

    peers.reserve(6);
    malicious_peers.reserve(6);

    genesis = nullptr;
    private_leaf = nullptr;

    transactions_received = 0;
    blocks_received = 0;
}

void Node::create_transaction()
{
    // randomly choose amount and receiver
    int receiver = uniform_distribution(0, number_of_nodes - 1);
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

void Node::send_get_to_link(const shared_ptr<Block>& blk, Link &link) const
{
    get_block_request_object gobj(id,link.peer,blk);
    const long long latency = link.propagation_delay + get_message_size/link.link_speed + \
    exponential_distribution(static_cast<double>(queuing_delay_constant)/static_cast<double>(link.link_speed));
    event_queue.emplace(simulation_time + latency,GET_BLOCK_REQUEST,gobj);
}

void Node::receive_hash(const receive_hash_object &obj)
{
    if (block_ids_in_tree.count(obj.blk->id) == 1)
        return;

    if (hashes_seen.count(obj.blk->id) == 0)
    {
        hashes_seen.insert(obj.blk->id);

        Link to_send_link(1,1,1);
        Network& network = Network::getInstance();
        // if attacker overlay exists send through that
        if (malicious && network.nodes[obj.sender_node_id].malicious)
        {
            for (const auto& link: malicious_peers)
                if (link.peer == obj.sender_node_id)
                {
                    to_send_link = link;
                    break;
                }
        }
        else
        {
            for (const auto& link: peers)
                if (link.peer == obj.sender_node_id)
                {
                    to_send_link = link;
                    break;
                }
        }

        send_get_to_link(obj.blk,to_send_link);

        // Add timer
        Timer t(obj.blk,true);
        t.current_sender = obj.sender_node_id;
        t.tried_senders.insert(obj.sender_node_id);
        timers.emplace(obj.blk->id,t);

        // Generate timer expired event;
        timer_expired_object tobj(id,obj.blk);
        event_queue.emplace(simulation_time+ timer_timeout_time, TIMER_EXPIRED,tobj);
    }
    else
    {
        if (auto it = timers.find(obj.blk->id); it != timers.end())
        {
            it->second.available_senders.push(obj.sender_node_id);

            if (  !it->second.is_running)
            {
                timer_expired_object tobj(id,obj.blk);
                event_queue.emplace(simulation_time+ timer_timeout_time, TIMER_EXPIRED,tobj);

            }
        }
    }
}

void Node::timer_expired(const timer_expired_object& obj)
{
    auto it = timers.find(obj.blk->id);
    if (it == timers.end()) return;
    if (it->second.available_senders.empty())
    {
        it->second.is_running = false;
        return;
    }

    Link to_punish_link(1,1,1);

    for (auto link : peers)
    {
        if (link.peer == it->second.current_sender)
        {
            link.failed++;
            to_punish_link = link;
            break;
        }
    }

    if (to_punish_link.failed > 10 && mitigation)
    {
        // Remove the link from peers if failed count exceeds 10.
        peers.erase(
            std::remove_if(peers.begin(), peers.end(), [&](const auto& link) {
                return link.peer == it->second.current_sender;
            }),
            peers.end());


        // Add random peer
        Network& network = Network::getInstance();
        int new_node = choose_neighbours_values(network.honest_node_ids,1 , {id})[0];

        int link_speed = network.nodes[id].fast && network.nodes[new_node].fast ? 100 * 1000 : 5 * 1000; // bits per millisecond

        int propagation_delay = uniform_distribution(propagation_delay_min,propagation_delay_max);
        network.nodes[id].peers.emplace_back(new_node, propagation_delay, link_speed);
        network.nodes[new_node].peers.emplace_back(id, propagation_delay, link_speed);
    }


    // send to the next available
    int next_sender = it->second.available_senders.front();
    it->second.available_senders.pop();
    while (it->second.tried_senders.count(next_sender)!=0 && !it->second.available_senders.empty())
    {
        next_sender = it->second.available_senders.front();
        it->second.available_senders.pop();
    }

    // send get request to next sender
    it->second.tried_senders.insert(next_sender);

    Link to_send_link(1,1,1);
    Network& network = Network::getInstance();
    // if attacker overlay exists send through that
    if (malicious && network.nodes[next_sender].malicious)
    {
        for (const auto& link: malicious_peers)
            if (link.peer ==next_sender)
            {
                to_send_link = link;
                break;
            }
    }
    else
    {
        for (const auto& link: peers)
            if (link.peer == next_sender)
            {
                to_send_link = link;
                break;
            }
    }
    send_get_to_link(obj.blk,to_send_link);
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
        if (obj.tries > maximum_retries)
            return;

        l.log <<"Time "<< simulation_time << ": Node " << id << " NACK block  "<<obj.blk->id<<endl;
        // simulate resending the block by the peer (assume ACK, NACK mechanism)


        Link to_send_link(1,1,1);
        Network& network = Network::getInstance();
        // if attacker overlay exists send through that
        if (malicious && network.nodes[obj.sender_node_id].malicious)
        {
            for (const auto& link: malicious_peers)
                if (link.peer == obj.sender_node_id)
                {
                    to_send_link = link;
                    break;
                }
        }
        else
        {
            for (const auto& link: peers)
                if (link.peer == obj.sender_node_id)
                {
                    to_send_link = link;
                    break;
                }
        }

        const long long size = transaction_size * static_cast<long long>(obj.blk->transactions.size());
        const long long latency = to_send_link.propagation_delay + size/to_send_link.link_speed + \
        exponential_distribution(static_cast<double>(queuing_delay_constant)/static_cast<double>(to_send_link.link_speed));
        Event e(simulation_time + latency,RECEIVE_BLOCK,receive_block_object(obj.sender_node_id,obj.receiver_node_id,obj.blk,obj.tries+1));
        event_queue.push(e);

        return;
    }

    // if validated and added to the longest chain, re-start mining on longest chain
    if (validate_and_add_block(obj.blk))
    {
        l.log << "Time "<< simulation_time <<": Node " << id << " block  "<<obj.blk->id<< " extended longest chain" << endl;

        // remove corresponding timer
        auto it = timers.find(obj.blk->id);
        if (it != timers.end()) timers.erase(it);


        if (!malicious)
        {
            mine_block();
            return;
        }

        if (selfish_mining && ringmaster && obj.blk->is_private)
        {
            mine_block();
            return;
        }

        if (selfish_mining && ringmaster && !obj.blk->is_private)
        {
            long long global_length = (*leaves.begin())->length;
            long long private_length = private_leaf==nullptr? 0 : private_leaf->length;

            printf("Global : %lld Private %lld Generated by %d  block id : %lld parend id: %lld \n",global_length,private_length,(*obj.blk->transactions.begin())->receiver, obj.blk->id,obj.blk->parent_block->id);

            if (global_length == private_length -1 || global_length == private_length)
            {
                global_send_private_counter++;
                long long private_leaf_id = private_leaf == nullptr? -1 : private_leaf->block->id;
                release_private(global_send_private_counter);
                printf("released private chain, private_leaf: %lld  honest_block: %lld \n", private_leaf_id, obj.blk->id);
                // mine_block();
            }
        }
    }
}

bool Node::validate_and_add_block(shared_ptr<Block> blk)
{
    vector<long long > temp_balance;
    set<long long > temp_transaction_ids;
    long long temp_length =1;
    const auto it = find_if(leaves.begin(),leaves.end(),\
            [&blk](const shared_ptr<LeafNode>& leaf){return blk->parent_block->id == leaf->block->id;});


    if (selfish_mining && malicious && private_leaf != nullptr && blk->is_private )
    {
        temp_balance = private_leaf->balance;
        temp_transaction_ids = private_leaf->transaction_ids;
        temp_length = private_leaf->length+1;
    }
    else
    {
        // check if the parent node of the block is leaf node
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
    broadcast_hash(blk);

    if (malicious || !blk->is_private)
        block_ids_in_tree.insert({blk->id,simulation_time});

    l.log << "Time "<< simulation_time <<": Node " << id << " successfully validated block  "<<blk->id<<endl;

    // Create leaf node
    const auto temp_leaf = make_shared<LeafNode>(blk,temp_length);
    temp_leaf->balance= std::move(temp_balance);
    temp_leaf->transaction_ids = std::move(temp_transaction_ids);


    if ( selfish_mining && malicious && blk->is_private)
    {

        private_leaf = temp_leaf;

        long long global_length = (*leaves.begin())->length;
        long long private_length = private_leaf==nullptr? 0 : private_leaf->length;

        printf("Global : %lld Private %lld Generated by %d  block id : %lld parent id: %lld \n",global_length,private_length,(*blk->transactions.begin())->receiver,blk->id,blk->parent_block->id);
        return true;

    }

    else
    {
        // return true if longest changes after inserting
        const long long previous_longest = (*leaves.begin())->block->id;
        if (it != leaves.end())
            leaves.erase(it);
        leaves.insert(temp_leaf);
        const long long current_longest = (*leaves.begin())->block->id;

        return (previous_longest != current_longest);
    }
}

void Node::broadcast_hash(const shared_ptr<Block>& blk)
{
    if (malicious)
    {
        for (auto link : malicious_peers)
        {
            // send hash if not already sent to the peer
            if (link.hash_sent.count(blk->id) == 0)
            {
                link.hash_sent.insert(blk->id);
                const long long latency = link.propagation_delay + hash_size/link.link_speed + \
                exponential_distribution(static_cast<double>(queuing_delay_constant)/static_cast<double>(link.link_speed));

                // create receive hash event for that node at current time + latency
                long long hash_value = compute_hash(blk);
                receive_hash_object obj(hash_value,id,link.peer,blk);
                Event e(simulation_time + latency,RECEIVE_HASH,obj);
                event_queue.push(e);
            }
        }
    }

    if (!blk->is_private)
    {
        for (auto link : peers)
        {
            // send hash if not already sent to the peer
            if (link.hash_sent.count(blk->id) == 0)
            {
                link.hash_sent.insert(blk->id);
                const long long latency = link.propagation_delay + hash_size/link.link_speed + \
                exponential_distribution(static_cast<double>(queuing_delay_constant)/static_cast<double>(link.link_speed));

                // create receive hash event for that node at current time + latency
                long long hash_value = compute_hash(blk);
                // cout<<"hash value :"<<hash_value <<"Block id :"<<blk->id<<endl;
                receive_hash_object obj(hash_value,id,link.peer,blk);
                Event e(simulation_time + latency,RECEIVE_HASH,obj);
                event_queue.push(e);
            }
        }
    }
}

void Node::mine_block()
{
    currently_mining = true;
    if (mempool.empty() || hashing_power == 0)
    {
        currently_mining = false;
        return;
    }
    // create the new block with coinbase transaction
    shared_ptr<LeafNode> longest_leaf = *leaves.begin();

    if (selfish_mining && ringmaster && private_leaf!= nullptr)
    {
        longest_leaf = private_leaf;
    }

    auto blk = make_shared<Block>(simulation_time,longest_leaf->block,ringmaster,!ringmaster);
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
    const double hashing_fraction = static_cast<double>(hashing_power)/static_cast<double>(number_of_nodes);
    const long long mining_time = exponential_distribution(static_cast<double>(block_inter_arrival_time)/hashing_fraction);
    block_mined_object obj(id,blk);
    event_queue.emplace(simulation_time + mining_time,BLOCK_MINED, obj);
}

void Node::complete_mining(const shared_ptr<Block>&  blk)
{
    shared_ptr<LeafNode> longest_leaf = *leaves.begin();

    if (selfish_mining && ringmaster && private_leaf!= nullptr || blk->parent_block->id == longest_leaf->block->id)
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

void Node::send_block(const get_block_request_object &obj){

    Network& network = Network::getInstance();

    if (eclipse_attack && malicious && !network.nodes[obj.sender_node_id].malicious && obj.blk->is_honest)
        return;

    const long long size = (transaction_size) * static_cast<long long>(obj.blk->transactions.size());

    Link to_send_link(1,1,1);
    // if attacker overlay exists send through that
    if (malicious && network.nodes[obj.sender_node_id].malicious)
    {
        for (const auto& link: malicious_peers)
            if (link.peer == obj.sender_node_id)
            {
                to_send_link = link;
                break;
            }
    }
    else
    {
        for (const auto& link: peers)
            if (link.peer == obj.sender_node_id)
            {
                to_send_link = link;
                break;
            }
    }
        const long long latency = to_send_link.propagation_delay + size/to_send_link.link_speed + \
        exponential_distribution(static_cast<double>(queuing_delay_constant)/static_cast<double>(to_send_link.link_speed));

        // create receive block event for that node at current time + latency
        receive_block_object robj(id,to_send_link.peer,obj.blk,0);
        Event e(simulation_time + latency,RECEIVE_BLOCK,robj);
        event_queue.push(e);
}

long long Node::compute_hash(shared_ptr<Block> blk)
{
    //TODO: Compute actual hash and return
    // stringstream ss;
    // ss << blk->id << "-" 
    //    << (blk->parent_block ? blk->parent_block->id : 0) << "-"
    //    << blk->creation_time << "-"
    //    << blk->is_private << "-"
    //    << blk->is_honest;
    
    // string hash_str = md5(ss.str());
    
    // long long hash_val = 0;
    // for (size_t i = 0; i < 16 && i < hash_str.size(); i++) {
    //     hash_val = (hash_val << 4) | (hash_str[i] % 16);
    // }
   
    // return hash_val;
    return blk->id;
}

void Node::release_private_helper(shared_ptr<Block> blk)
{
    if ( blk == nullptr)
        return;
    release_private_helper(blk->parent_block);

    blk->is_private = false;
    broadcast_hash(blk);
}

void Node::release_private(int counter)
{
    if (private_leaf == nullptr)
        return;

    for (auto link : malicious_peers)
    {
        // send hash if not already sent to the peer
        if (link.release_private_sent.count(counter) == 0)
        {
            link.release_private_sent.insert(counter);

            const long long latency = link.propagation_delay + get_message_size/link.link_speed + \
            exponential_distribution(static_cast<double>(queuing_delay_constant)/static_cast<double>(link.link_speed));

            // create receive hash event for that node at current time + latency
            release_private_object obj(link.peer,counter);
            Event e(simulation_time + latency,RELEASE_PRIVATE,obj);
            event_queue.push(e);
        }
    }
    release_private_helper(private_leaf->block);
    leaves.insert(private_leaf);
    private_leaf = nullptr;
}


Network& Network::getInstance()
{
    static Network instance;
    return instance;
}

void Network::build_network(vector<int> &node_ids,const string& networkType){
    bool done = false;
    map<int, vector<int>> mal;  // adjacency list as a map

    for (int node_idx : node_ids) {
        mal[node_idx] = {};
    }


    // until graph is connected
    while (!done) {
        mal.clear();
        for (int i = 0; i < node_ids.size(); i++) {
            int node_idx = node_ids[i];
            int min_peers = min(3, static_cast<int>(node_ids.size() - 1));

            // until at least connected to min peers, keep adding neighbors
            while (mal[node_idx].size() < min_peers) {

                // create excluded list for possible neighbours
                vector<int> excluded;
                excluded.push_back(node_idx);
                for (int j = 0; j < node_ids.size(); j++) {
                    if (find(mal[node_idx].begin(), mal[node_idx].end(), node_ids[j]) != mal[node_idx].end()) {
                        excluded.push_back(node_ids[j]);
                    }
                }

                // choose new neighbors
                vector<int> temp = choose_neighbours_values(
                    node_ids,
                    min_peers - static_cast<int>(mal[node_idx].size()),
                    excluded
                );


                for (auto neighbour : temp) {
                    int nei_idx = neighbour;

                    // ensure max 6 peers, no self-connection, and no duplicate edges
                    if (nei_idx != node_idx &&
                        mal[nei_idx].size() < min(6, static_cast<int>(node_ids.size() - 1)) &&
                        find(mal[node_idx].begin(), mal[node_idx].end(), nei_idx) == mal[node_idx].end()) {
                        mal[node_idx].push_back(nei_idx);
                        mal[nei_idx].push_back(node_idx);
                    }
                }

            }
        }

        done = check_connected_map(mal);  // check if the graph is connected

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

                int link_speed = nodes[i].fast && nodes[x].fast ? 100 * 1000 : 5 * 1000; // bits per millisecond

                if (networkType == "common"){
                    int propagation_delay = uniform_distribution(propagation_delay_min,propagation_delay_max);
                    nodes[i].peers.emplace_back(x, propagation_delay, link_speed);
                    nodes[x].peers.emplace_back(i, propagation_delay, link_speed);
                }
                else{
                    int propagation_delay = uniform_distribution(propagation_delay_malicious_min,propagation_delay_malicious_max); // 1ms to 10ms
                    nodes[i].malicious_peers.emplace_back(x, propagation_delay, link_speed);
                    nodes[x].malicious_peers.emplace_back(i, propagation_delay, link_speed);
                }
            }
        }
    }
    cout<<"Network built for "<<networkType<<endl;

}


Network::Network()
{
    // Node id equal to its index in vector
    nodes.resize(number_of_nodes);
    vector<int> all_node_ids;
    for (int i=0; i<number_of_nodes; i++){
        all_node_ids.push_back(i);
    }

    //   create honest and malicious nodes ids and their nodes_ptr instances
    malicious_node_ids = choose_percent(number_of_nodes, static_cast<double>(percent_malicious_nodes) / static_cast<double>(100));
    bool assigned_ringmaster = false;
    for(int i=0; i<number_of_nodes; i++){
        if (find(malicious_node_ids.begin(), malicious_node_ids.end(), i) != malicious_node_ids.end()){
            if (assigned_ringmaster){
                nodes[i].malicious = true;
                nodes[i].hashing_power = 0;
                nodes[i].fast = true;
            }
            else{
                nodes[i].malicious = true;
                nodes[i].ringmaster = true;
                nodes[i].fast = true;
                nodes[i].hashing_power =static_cast<long long> (malicious_node_ids.size());

                assigned_ringmaster = true;
                cout<<"Ringmaster id: " << nodes[i].id<<endl;
                ringmaster_node_id = nodes[i].id;
            }
        }
        else{
            honest_node_ids.push_back(i);
            nodes[i].hashing_power = 1;
        }

    }


    build_network(all_node_ids, "common");
    build_network(malicious_node_ids, "malicious");

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}

// Logger::Logger()
// {
//     if (fs::path dir = output_dir +"/Log"; !fs::exists(dir))
//         fs::create_directories(dir);
//     log.open(output_dir + "/Log/log.txt");
// }

// Logger::~Logger()
// {
//     log.close();
// }

// void Logger::setOutputDir(const std::string& dir)
// {
//     output_dir = dir;

//     if (fs::path log_dir = output_dir + "/Log"; !fs::exists(log_dir))
//     {
//         fs::create_directories(log_dir);
//     }

//     log.open(output_dir + "/Log/log.txt", std::ios::out);

//     if (!log.is_open())
//     {
//         std::cerr << "Error: Unable to open log file at " << output_dir + "/Log/log.txt" << std::endl;
//         exit(1);
//     }
// }

Logger::Logger() 
{
    // constructor should not attempt to open the log file.
    // Log initialization will be deferred to the setOutputDir method.
}

Logger::~Logger()
{
    if (log.is_open())
    {
        log.close();
    }
}

void Logger::setOutputDir(const std::string& dir)
{
    output_dir = dir;

    // Create the directory if it doesn't already exist
    if (fs::path log_dir = output_dir + "/Log"; !fs::exists(log_dir))
    {
        fs::create_directories(log_dir);
    }

    // Open the log file in the specified directory
    log.open(output_dir + "/Log/log.txt", std::ios::out);

    if (!log.is_open())
    {
        std::cerr << "Error: Unable to open log file at " << output_dir + "/Log/log.txt" << std::endl;
        exit(1);
    }
}
