#include "Simulator.h"

#include <algorithm>
#include <numeric>


void Simulator::create_genesis()
{
    auto genesis = make_shared<Block>(0, nullptr,false,true);
    // create coinbase transaction for each node with initial bitcoin
    for (int i = 0; i < number_of_nodes; i++)
    {
        auto temp = make_shared<Transaction>(i, initial_bitcoin, true);
        genesis->transactions.push_back(temp);
    }

    cout << " Created genesis block" << endl;
    // add genesis block to all nodes
    for (int i = 0; i < number_of_nodes; i++)
    {
        network.nodes[i].genesis = genesis;
        network.nodes[i].block_ids_in_tree.insert({genesis->id, simulation_time});

        // update leaves in each node with initial balance, transaction ids and balance of each node
        auto temp = make_shared<LeafNode>(genesis, 1);
        for (const auto& txn : genesis->transactions)
        {
            temp->transaction_ids.insert(txn->id);
            if (txn->coinbase) temp->balance[txn->receiver] += txn->amount;
            else
            {
                if (temp->balance[txn->sender] - txn->amount < 0)
                    throw std::runtime_error("Invalid genesis block");

                temp->balance[txn->sender] -= txn->amount;
                temp->balance[txn->receiver] += txn->amount;
            }
        }
        network.nodes[i].leaves.insert(temp);
    }
    cout << " Added genesis block to all nodes" << endl;
}

// Populate the event queue with initial transactions
void Simulator::create_initial_transactions()
{

    long long event_time = 0; // variable to keep track of future time

    create_transaction_object obj(network.ringmaster_node_id);
    auto e = Event(0,CREATE_TRANSACTION, obj);
    event_queue.push(e);

    for (int i = 0; i < initial_number_of_transactions; i++)
    {
        // randomly choose node who creates transaction
        int creator_node_id = uniform_distribution(0, number_of_nodes - 1);
        // create transaction object and put into  event_queue
        create_transaction_object obj(creator_node_id);
        event_time += exponential_distribution(mean_transaction_inter_arrival_time);
        auto e = Event(event_time,CREATE_TRANSACTION, obj);
        event_queue.push(e);

        create_transaction_object objr(network.ringmaster_node_id);
        event_time += exponential_distribution(mean_transaction_inter_arrival_time);
        auto er = Event(event_time,CREATE_TRANSACTION, objr);
    }
    cout << " Initialized event queue with " << initial_number_of_transactions << " transactions" << endl;
}

void Simulator::initialize()
{
    // create genesis blocks and add it to all nodes
    create_genesis();

    // create initial set of transactions and put it into event queue;
    create_initial_transactions();
}

void Simulator::start()
{
    bool flag = true;
    cout << " Simulation started" << endl;
    // Process each type of event in event queue
    while (!event_queue.empty())
    {
        // get the event and update the simulation clock
        Event e = event_queue.top();
        event_queue.pop();
        simulation_time = e.time;

        if (e.type == CREATE_TRANSACTION)
        {
            const auto obj = std::get<struct create_transaction_object>(e.object);
            network.nodes[obj.creator_node_id].create_transaction();
        }

        else if (e.type == RECEIVE_TRANSACTION)
        {
            auto obj = std::get<struct receive_transaction_object>(e.object);
            network.nodes[obj.receiver_node_id].receive_transaction(obj);
        }
        else if (e.type == RECEIVE_BLOCK)
        {
            const auto obj = std::get<struct receive_block_object>(e.object);
            network.nodes[obj.receiver_node_id].receive_block(obj);
        }

        else if (e.type == BLOCK_MINED)
        {
            const auto obj = std::get<struct block_mined_object>(e.object);
            network.nodes[obj.miner_node_id].complete_mining(obj.blk);
        }

        else if (e.type == RECEIVE_HASH)
        {
            const auto obj = std::get<struct receive_hash_object>(e.object);
            network.nodes[obj.receiver_node_id].receive_hash(obj);
        }

        else if (e.type == GET_BLOCK_REQUEST)
        {
            const auto obj = std::get<struct get_block_request_object>(e.object);
            network.nodes[obj.receiver_node_id].send_block(obj);

        }

        else if (e.type == TIMER_EXPIRED)
        {
            const auto obj = std::get<struct timer_expired_object>(e.object);
            network.nodes[obj.node_id].timer_expired(obj);
        }

        else if (e.type == RELEASE_PRIVATE)
        {
            const auto obj = std::get<struct release_private_object>(e.object);
            network.nodes[obj.node_id].release_private(obj.counter);
        }

        if (event_queue.empty() && flag)
        {
            flag = false;
            network.nodes[network.ringmaster_node_id].release_private(global_send_private_counter++);
        }
    }
    cout << " Simulation completed, Writing stats to files" << endl;

    // Write stats file
    write_node_stats_to_file();
    write_all_node_details_to_file(network.nodes, "all_node_details.csv");
    cout << " Stats written in ./files/ directory" << endl;
    cout << " Logs written in ./files/logs.txt" << endl;
}

void Simulator::write_node_stats_to_file()
{
    // Check if the directory exists, if not create it
    if (fs::path dir = output_dir + "/Node_Files"; !fs::exists(dir))
        fs::create_directories(dir);


    ofstream file;
    for (int i = 0; i < number_of_nodes; i++)
    {
        string filepath = output_dir + "/Node_Files/Node_" + std::to_string(i) + ".txt";
        file.open(filepath);

        if (!file)
        {
            cerr << "An Error occurred while opening file!" << endl;
            return;
        }

        // if (network.nodes[i].private_leaf != nullptr)
        //     network.nodes[i].leaves.insert(network.nodes[i].private_leaf);

        file << "Node ID: " << network.nodes[i].id << endl;
        file << "Fast node: " << network.nodes[i].fast << endl;
        file << "Peers:" << endl;
        for (auto& peer : network.nodes[i].peers)
        {
            file << "\t Node id: " << peer.peer << " Propagation delay: " << peer.propagation_delay << " ms"
                << " Link speed: " << peer.link_speed << endl;
        }
        file << "Transactions received: " << network.nodes[i].transactions_received << endl;
        file << "Blocks received: " << network.nodes[i].blocks_received << endl;

        long long blocks_created = 0;
        long long blocks_in_longest_chain = 0;
        long long created_blocks_in_longest_chain = 0;
        set<block_stats> blocks; // block statistics to be sorted by first seen time
        set<long long> block_ids; // set to keep track of already inserted blocks


        long long longest = (*network.nodes[i].leaves.begin())->block->id;

        // variables for counting fork statistics
        vector<long long> fork_lengths;
        long long temp_fork_length = 0;
        set< long long> block_ids_in_longest;

        // traverse other chains and get block stats
        for (const auto& leaf : network.nodes[i].leaves)
        {
            bool longest_flag = false;
            if (leaf->block->id == longest)
                longest_flag = true;

            shared_ptr<Block> temp_block = leaf->block;
            while (temp_block)
            {
                // if already reached block in the longest return
                if (block_ids_in_longest.count(temp_block->id) != 0)
                {
                    fork_lengths.push_back(temp_fork_length);
                    temp_fork_length = 0;
                    break;
                }

                // if block visited already just increment fork length
                if (block_ids.count(temp_block->id) != 0)
                {
                    temp_fork_length++;
                    temp_block = temp_block->parent_block;
                    continue;
                }

                block_stats b;
                b.block_id = temp_block->id;
                if (temp_block->parent_block)
                    b.parent_block_id = temp_block->parent_block->id;
                else
                    b.parent_block_id = -1;
                b.first_seen_time = network.nodes[i].block_ids_in_tree[temp_block->id];
                b.num_transactions = static_cast<long long>(temp_block->transactions.size());
                block_ids.insert(b.block_id);

                if (longest_flag)
                {
                    b.part_of_longest = true;
                    blocks_in_longest_chain++;
                    block_ids_in_longest.insert(temp_block->id);
                }
                else
                {
                    temp_fork_length++;
                    b.part_of_longest = false;
                }

                if (temp_block->transactions[0]->receiver == i && temp_block != network.nodes[i].genesis)
                {
                    b.generated_by_node = true;
                    blocks_created++;
                    if (longest_flag) created_blocks_in_longest_chain++;
                }

                blocks.insert(b);
                temp_block = temp_block->parent_block;
            }
        }

        file << "Blocks mined: " << blocks_created << endl;
        file << "Blocks mined in longest chain: " << created_blocks_in_longest_chain << endl;
        file << "Number of blocks in longest chain: " << blocks_in_longest_chain << endl;
        file << "Fraction of blocks mined in longest chain: " <<
            static_cast<double>(created_blocks_in_longest_chain) / static_cast<double>(blocks_in_longest_chain) << endl;
        if ( blocks_created !=0 )
            file << "Fraction of mined blocks not in longest chain: " << static_cast<double>(blocks_created - created_blocks_in_longest_chain)/static_cast<double> (blocks_created)
        << endl;
        else
            file << "Fraction of mined blocks not in longest chain: " << 0 << endl;

        file << "Number of forks: " << fork_lengths.size() << endl;

        unsigned long long average_fork_length = 0;
        long long shortest_fork_length = 0;
        long long longest_fork_length = 0;

        if (!fork_lengths.empty())
        {
            average_fork_length  = accumulate(fork_lengths.begin(), fork_lengths.end(), 0LL) / fork_lengths.size();
            longest_fork_length  = *max_element(fork_lengths.begin(), fork_lengths.end());
            shortest_fork_length = *min_element(fork_lengths.begin(), fork_lengths.end());
        }
        file << "Longest fork length:" << longest_fork_length << endl;
        file << "Shortest fork length:" << shortest_fork_length << endl;
        file << "Average fork length: " << average_fork_length << endl;
        file << "Blockchain: " << endl;
        file << "Block_id, parent_block_id, first_seen_time, number_of_transactions, part_of_longest, mined by node" <<
            endl;
        for (auto x : blocks)
        {
            file << x.block_id << ", " << x.parent_block_id << ", " << x.first_seen_time << ", " << x.num_transactions
                << ", " << x.part_of_longest
                << ", " << x.generated_by_node << endl;
        }
        file.close();
    }
}

void Simulator::write_all_node_details_to_file(const vector<Node>& nodes, const std::string &fname)
{
    fs::path dir = output_dir + "/Temp_files/";

    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    std::string filepath = output_dir + "/Temp_files/" + fname;
    std::ofstream file(filepath);

    if (!file) {
        std::cerr << "An Error occurred while opening file!" << std::endl;
        return;
    }

    // CSV Header
    file << "node_id,malicious,ringmaster,fast,hashing_power,hashing_fraction,num_peers_in_common,num_peers_in_overlay" << std::endl;

    for (const auto& node : nodes) {
        file << node.id << ","
             << node.malicious << ","   // 1 if MaliciousNode, 0 otherwise
             << node.ringmaster << ","  // 1 if RingMasterNode, 0 otherwise
             << node.fast << ","
             << node.hashing_power << ","
             << static_cast<double>(node.hashing_power) / static_cast<double>(number_of_nodes) << ","
             << node.peers.size() << ","
             << node.malicious_peers.size() << "," << endl;
    }

    file.close();
}

bool block_stats::operator<(const block_stats& other) const
{
    if (first_seen_time == other.first_seen_time) return block_id < other.block_id;
    return first_seen_time < other.first_seen_time;
}


block_stats::block_stats()
{
    block_id = -1;
    parent_block_id = -1;
    first_seen_time = -1;
    num_transactions = -1;
    part_of_longest = false;
    generated_by_node = false;
}

