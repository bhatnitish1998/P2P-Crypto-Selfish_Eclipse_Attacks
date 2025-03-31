// time : milliseconds
// data : bits
// amount : bitcoin

#include "Network.h"
#include "Simulator.h"
#include "Event.h"
#include <cstdlib>
#include <fstream>

// experiment constants
int initial_bitcoin = 1000;
int initial_number_of_transactions = 20000;
int propagation_delay_min = 10;
int propagation_delay_max = 500;
int propagation_delay_malicious_min = 1;
int propagation_delay_malicious_max = 10;
int transaction_amount_min = 5;
int transaction_amount_max = 20;
int queuing_delay_constant = 96;  // 96000 per second
int transaction_size = 1024 * 8;
int hash_size = 64*8;
int get_message_size = 64*8;
int mining_reward = 50;
int maximum_retries = 100;

// experiment parameters
int number_of_nodes;
int percent_malicious_nodes;
int mean_transaction_inter_arrival_time;
int block_inter_arrival_time;
int timer_timeout_time;
string output_dir = "Output";

// Simulation variables
long long simulation_time = 0;
EQ event_queue;
unsigned int global_seed = 911;
Logger l;
bool selfish_mining = true;
bool eclipse_attack = false;
int global_send_private_counter =0;


int main(int argc, char* argv[])
{
    if (argc < 7 || argc > 8)
    {
        cerr << "Usage: " << argv[0] <<
            " <number_of_nodes> <percent_malicious> <mean_transaction_inter_arrival_time> <block_inter_arrival_time> <timeout time> <output_dir> [--eclipse]" 
            << endl;
        cerr << "  mean_transaction_inter_arrival_time: milli-seconds" << endl;
        cerr << "  block_inter_arrival_time: seconds" << endl;
        cerr << "  timeout time: milli-seconds" << endl;
        cerr << "  output_dir" << endl;
        cerr << "  [--eclipse]: optional argument to enable eclipse attack" << endl;
        return 1;
    }

    number_of_nodes = stoi(argv[1]);
    percent_malicious_nodes = stoi(argv[2]);
    mean_transaction_inter_arrival_time = stoi(argv[3]);
    block_inter_arrival_time = stoi(argv[4])* 1000 ;
    timer_timeout_time = stoi(argv[5]);
    output_dir = argv[6];

    l.setOutputDir(output_dir);

    if (argc == 8 && string(argv[7]) == "--eclipse")
        eclipse_attack = true;

    if (number_of_nodes < 1 ||  percent_malicious_nodes < 0 || percent_malicious_nodes > 100
        || mean_transaction_inter_arrival_time <= 0 || block_inter_arrival_time <= 0 || timer_timeout_time <= 0)
    {
        cerr << "Invalid argument values" << endl;
        return 1;
    }

    // Print experiment configuration
    cout << "----------------------------------------------------------------------" << endl;
    cout << "Simulation Configuration:" << endl;
    cout << "  Number of Nodes: " << number_of_nodes << endl;
    cout << "  Percent Malicious: " << percent_malicious_nodes << "%" << endl;
    cout << "  Mean Transaction Inter-Arrival Time: " << mean_transaction_inter_arrival_time << " ms" << endl;
    cout << "  Block Inter-Arrival Time: " << block_inter_arrival_time << " ms" << endl;
    cout << "  Timeout Time: " << timer_timeout_time << " ms" << endl;

    cout << "  Propagation Delay (Min-Max): " << propagation_delay_min << " - " << propagation_delay_max << " ms" <<
        endl;
    cout << "  Propagation Delay Malicious (Min-Max): " << propagation_delay_malicious_min << " - " << propagation_delay_malicious_max << " ms" << endl;
    cout << "  Queuing Delay Constant: " << queuing_delay_constant << " bits/sec" << endl;
    cout << "  Transaction Size: " << transaction_size << " bits" << endl;
    cout << "  Hash size: " << hash_size << endl;
    cout << "  Get message size: " << get_message_size << endl;
    cout << "  Mining reward: " << mining_reward << " bitcoins" << endl;
    cout << "  Initial Bitcoins with each node: " << initial_bitcoin << endl;
    cout << "  Initial Number of Transactions: " << initial_number_of_transactions << endl;
    cout << "  Transaction Amount (Min-Max): " << transaction_amount_min << " - " << transaction_amount_max <<
        " bitcoins" << endl;
    cout << "  Eclipse Attack: " << (eclipse_attack ? "Enabled" : "Disabled") << endl;
    cout << "  Selfish Mining: " << (selfish_mining ? "Enabled" : "Disabled") << endl;
    cout << "  Output Directory: " << output_dir << endl;
    cout << "----------------------------------------------------------------------" << endl;
    srand(global_seed);

    // Create and start simulation
    Simulator sim;
    sim.initialize();
    sim.start();

    return 0;
}
