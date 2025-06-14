#ifndef UTILITY_H
#define UTILITY_H

#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <map>

using namespace std;
namespace fs = filesystem;

extern unsigned int global_seed;
extern string output_dir;


// min and max are inclusive
int uniform_distribution(int min, int max);

// returns discrete values (milliseconds)
int exponential_distribution(double mean);

// returns percentage of nodes from given 0 to n-1 nodes
vector<int> choose_percent(int n, double percent);

// returns k nodes apart from those in <excluded> from 0 to n-1 nodes.
vector<int> choose_neighbours(int n, int k, vector<int> excluded);

vector<int> choose_neighbours_values(vector<int> universe_set, int k, vector<int> excluded);

// checks if the given graph is connected using dfs
bool check_connected(vector<vector<int>>& al);

bool check_connected_map(map<int, vector<int>>& al);

// Creates a file with name fname and write graphs nodes in it.
void write_network_to_file(vector<vector<int>>& al,const string& fname);

void write_network_to_file_map(map<int, vector<int>>& al,const string &fname);

//  Hash computation taken from MICA key-value store by Hyeontaek Lim
// string md5(const string &data);

#endif //UTILITY_H
