#include "utility_functions.h"
#include <vector>
#include <ctime>
#include <algorithm>
#include <stack>
#include <random>
#include <filesystem>
#include <openssl/evp.h>
#include <sstream>



// return random number from uniform distribution
int uniform_distribution(const int min, const int max)
{
    static std::mt19937 generator(global_seed); // Mersenne Twister generator
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

// returns discrete event timings from exponential distribution
int exponential_distribution(double mean)
{
    static std::mt19937 generator(global_seed); // Mersenne Twister generator
    std::exponential_distribution<double> distribution(1.0 / mean);
    return static_cast<int>(distribution(generator));
}

// returns percentage of nodes from given 0 to n-1 nodes
vector<int> choose_percent(const int n, const double percent)
{
    const int num_to_select = static_cast<int>(n * percent);
    vector<int> selected_nodes;
    selected_nodes.reserve(num_to_select);

    // incrementally choose nodes without repetition from uniform distribution
    while (selected_nodes.size() < num_to_select)
    {
        int candidate = uniform_distribution(0, n - 1);
        if (find(selected_nodes.begin(), selected_nodes.end(), candidate) == selected_nodes.end())
            selected_nodes.push_back(candidate);
    }
    return selected_nodes;
}

// return k node ids as vector randomly from |total nodes| - |excluded nodes|
vector<int> choose_neighbours(int n, int k, vector<int> excluded)
{
    vector<int> selected_nodes;
    selected_nodes.reserve(k);

    // incrementally choose nodes without repetition from uniform distribution
    while (selected_nodes.size() < k)
    {
        int candidate = uniform_distribution(0, n - 1);
        if (find(excluded.begin(), excluded.end(), candidate) == excluded.end() &&
            find(selected_nodes.begin(), selected_nodes.end(), candidate) == selected_nodes.end())
            selected_nodes.push_back(candidate);
    }
    return selected_nodes;
}

// return k node ids as vector randomly from given possible choices as the universe_set
vector<int> choose_neighbours_values(vector<int> universe_set, int k, vector<int> excluded) {

    int n = universe_set.size();
    vector<int> selected_nodes;
    selected_nodes.reserve(k);

    // incrementally choose nodes without repetition from uniform distribution
    while (selected_nodes.size() < k) {
        int candidate_idx = uniform_distribution(0, n - 1);
        int candidate = universe_set[candidate_idx];
        if (find(excluded.begin(), excluded.end(), candidate) == excluded.end() &&
            find(selected_nodes.begin(), selected_nodes.end(), candidate) == selected_nodes.end()) {
            selected_nodes.push_back(candidate);
        }
    }

    return selected_nodes;
}

// checks if the given graph is connected using dfs
bool check_connected(vector<vector<int>>& al)
{   
    int n = static_cast<int>(al.size());
    if (n == 0) return true;

    // initialize data structures
    vector<bool> visited(n, false);
    stack<int> s;
    s.push(0);
    visited[0] = true;
    int visited_count = 1;

    // dfs to determine all reachable nodes
    while (!s.empty())
    {
        const int node = s.top();
        s.pop();
        for (int neighbor : al[node])
        {
            if (!visited[neighbor])
            {
                visited[neighbor] = true;
                s.push(neighbor);
                visited_count++;
            }
        }
    }
    // check if all nodes reachable
    return visited_count == n;
}

// checks if the given graph is connected using dfs: ml is map
bool check_connected_map(map<int, vector<int>>& al) {
    static int cnt = 0;
    cnt++;
    cout << "Check connected map called " << cnt << " times" << endl;

    int n = static_cast<int>(al.size());
    if (n == 0) return true;

    // create a mapping from node ids to contiguous indices
    map<int, int> node_to_index;
    int index = 0;
    for (const auto& [node, _] : al) {
        node_to_index[node] = index++;
    }

    vector<bool> visited(n, false);
    stack<int> s;

    // start dfs from the first node in the map
    int start_node = al.begin()->first;
    s.push(start_node);
    visited[node_to_index[start_node]] = true;
    int visited_count = 1;

    // dfs to determine all reachable nodes
    while (!s.empty()) {
        const int node = s.top();
        s.pop();
        for (int neighbor : al[node]) {
            int neighbor_index = node_to_index[neighbor];
            if (!visited[neighbor_index]) {
                visited[neighbor_index] = true;
                s.push(neighbor);
                visited_count++;
            }
        }
    }

    // check if all nodes are reachable
    return visited_count == n;
}


// Creates a file with name fname and write graphs nodes in it.
void write_network_to_file(vector<vector<int>> &al,const string &fname)
{
    // directory name to store file
    fs::path dir = "Output/Temp_files/";

    // Check if the directory exists, if not create it
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    string filepath = "Output/Temp_files/" + fname;

    ofstream file(filepath);

    if(!file){
        cerr << "An Error occurred while opening file!" << endl;
        return;
    }
    
    // store adjacency list in file
     for (size_t node = 0; node < al.size(); node++) {
        for (int neighbor : al[node]) {
            if (node < neighbor) {
                file << node << " " << neighbor << "\n";
            }
        }
    }

    file.close();
}

// creates a file with name fname and write graphs nodes in it.
void write_network_to_file_map(map<int, vector<int>>& al,const string &fname)
{
    // directory name to store file
    fs::path dir = "Output/Temp_files/";

    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    string filepath = "Output/Temp_files/" + fname;
    ofstream file(filepath);
    if(!file){
        cerr << "An Error occurred while opening file!" << endl;
        return;
    }

    // store adjacency list in the main file (edge list format)
    for (const auto& [node, neighbors] : al) {
        for (int neighbor : neighbors) {
            if (node < neighbor) {  // avoid duplicate edges (e.g., 0-8 and 8-0)
                file << node << " " << neighbor << "\n";
            }
        }
    }

    file.close();

    fs::path path(filepath);
    std::string base_filename = path.stem().string();
    std::string adj_list_filepath = dir.string() + base_filename + "_adj_list.txt";

    std::ofstream adj_file(adj_list_filepath);
    if (!adj_file) {
        std::cerr << "An Error occurred while opening adjacency list file: " << adj_list_filepath << std::endl;
        return;
    }

    // Store adjacency list in the [node x: neighbours] format
    for (const auto& [node, neighbors] : al) {
        adj_file << "Node " << node << " : ";
        for (int neighbor : neighbors) {
            adj_file << neighbor << " ";
        }
        adj_file << "\n";
    }
    adj_file.close();

}

//  Hash computation taken from MICA key-value store by Hyeontaek Lim
string md5(const string &data) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
    EVP_DigestUpdate(ctx, data.c_str(), data.size());
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);
    
    stringstream ss;
    for (unsigned int i = 0; i < digest_len; i++) {
        ss << hex << setw(2) << setfill('0') << (int)digest[i];
    }
    return ss.str();
}