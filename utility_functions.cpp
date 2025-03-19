#include "utility_functions.h"
#include <vector>
#include <ctime>
#include <algorithm>
#include <stack>
#include <random>

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
    
    // Store adjacency list in file
     for (size_t node = 0; node < al.size(); node++) {
        for (int neighbor : al[node]) {
            if (node < neighbor) {
                file << node << " " << neighbor << "\n";
            }
        }
    }

    file.close();
}
