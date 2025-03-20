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
int initial_number_of_transactions = 5000;
int propagation_delay_min = 10;
int propagation_delay_max = 500;
int transaction_amount_min = 5;
int transaction_amount_max = 20;
int queuing_delay_constant = 96;  // 96000 per second
int transaction_size = 1024 * 8;
int mining_reward = 50;

// experiment parameters
int number_of_nodes;
int percent_fast;
int percent_high_cpu;
int percent_malicious_nodes;
int percent_fast_malicious;
int percent_high_cpu_malicious;
int mean_transaction_inter_arrival_time;
int block_inter_arrival_time;

// Simulation variables
long long simulation_time = 0;
long long total_hashing_power = 0;
long long total_hashing_power_malicious = 0;
EQ event_queue;
unsigned int global_seed = 911;
Logger l;


int main(int argc, char* argv[])
{
    if (argc != 6)
    {
        cerr << "Usage: " << argv[0] <<
            " <number_of_nodes> <percent_fast> <percent_high_cpu> <mean_transaction_inter_arrival_time> <block_inter_arrival_time>"
            << endl;
        cerr << "  mean_transaction_inter_arrival_time: milli-seconds" << endl;
        cerr << "  block_inter_arrival_time: seconds" << endl;
        return 1;
    }

    number_of_nodes = stoi(argv[1]);
    percent_fast = stoi(argv[2]);
    percent_high_cpu = stoi(argv[3]);
    mean_transaction_inter_arrival_time = stoi(argv[4]);
    block_inter_arrival_time = stoi(argv[5])* 1000 ;
    percent_malicious_nodes = 50;
    

    if (number_of_nodes < 1 || percent_fast < 0 || percent_fast > 100 || percent_high_cpu < 0 || percent_high_cpu > 100
        || mean_transaction_inter_arrival_time <= 0 || block_inter_arrival_time <= 0)
    {
        cerr << "Invalid argument values" << endl;
        return 1;
    }

    // Print experiment configuration
    cout << "----------------------------------------------------------------------" << endl;
    cout << "Simulation Configuration:" << endl;
    cout << "  Number of Nodes: " << number_of_nodes << endl;
    cout << "  Percent Fast: " << percent_fast << "%" << endl;
    cout << "  Percent High CPU: " << percent_high_cpu << "%" << endl;
    cout << "  Mean Transaction Inter-Arrival Time: " << mean_transaction_inter_arrival_time << " ms" << endl;
    cout << "  Block Inter-Arrival Time: " << block_inter_arrival_time << " ms" << endl;
    cout << "  Propagation Delay (Min-Max): " << propagation_delay_min << " - " << propagation_delay_max << " ms" <<
        endl;
    cout << "  Queuing Delay Constant: " << queuing_delay_constant << " bits/sec" << endl;
    cout << "  Transaction Size: " << transaction_size << " bits" << endl;
    cout << "  Mining reward: " << mining_reward << " bitcoins" << endl;
    cout << "  Initial Bitcoins with each node: " << initial_bitcoin << endl;
    cout << "  Initial Number of Transactions: " << initial_number_of_transactions << endl;
    cout << "  Transaction Amount (Min-Max): " << transaction_amount_min << " - " << transaction_amount_max <<
        " bitcoins" << endl;
    cout << "----------------------------------------------------------------------" << endl;
    srand(global_seed);

    // Create and start simulation
    Simulator sim;
    sim.initialize();
    sim.start();

    return 0;
}
