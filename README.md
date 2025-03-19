# P2P Cryptocurrency Network
NOTE:Explanation of design is mentioned in report and design.png only contains flowchart.  
## Introduction
This project is an introduction to blockchain and cryptocurrencies, implementing a peer-to-peer (P2P) network for simulating blockchain transactions and mining.  

## Compilation and Execution
Compiling Code: g++ *.cpp -o main  
Running program: ./main <number_of_nodes> <percent_fast> <percent_high_cpu> <mean_transaction_inter_arrival_time_in_ms> <block_inter_arrival_time_in_sec>  

Example:./main 10 50 30 100 600   

Output folder will be generated in directory P2P-CRYPTOCURRENCY-NETWORK/ which contains Log,NodeFiles and Temp_files folder.  
P2P-CRYPTOCURRENCY-NETWORK/    
├── Output/  
	└── Log/    
	└── Temp_files/   
	└── Node_Files/    


## Changing configuration
Modify experimental parameters in main.cpp under the // experiment constants comment section. Key variables include:  
 
initial_number_of_transactions  
queuing_delay_constant   
propagation_delay_min  
propagation_delay_max  

## Output Data

After the program completes u will see files folder which contains stats related to all nodes with all details.   
Example:Open Node_0.txt  
Node ID: 0  
Fast node: 1  
High_cpu: 0  
Peers:  
	 Node id: 1 Propagation delay: 93 ms Link speed: 5000  
	 Node id: 5 Propagation delay: 266 ms Link speed: 100000  
	 Node id: 3 Propagation delay: 276 ms Link speed: 5000  
Transactions received: 3000  
Blocks received: 573  
Blocks mined: 6  
Blocks mined in longest chain: 6  
Number of blocks in longest chain: 498  
Fraction of blocks mined in longest chain: 0.0120482  
Fraction of mined blocks not in longest chain: 0  
Number of forks: 82  
Longest fork length:1  
Shortest fork length:1  
Average fork length: 1  
Blockchain:  

Entire blockchain detail  


## Visualization Scripts (Network and Blockchains)
### Prerequisites
Extract scripts.zip into the project root directory  

Directory structure should be:  
P2P-CRYPTOCURRENCY-NETWORK/  
	├── Output/  
	└── scripts/  

Set up virtual environment(if needed):  

In directory P2P-CRYPTOCURRENCY-NETWORK run:  
python3 -m venv myenv  
source myenv/bin/activate  
cd scripts/  
pip3 install -r requirements.txt  

## Network Visualization
Run python3 generate_p2p_network_graph.py to create p2p network graph.  
Output will be stored in Output/Network folder  

## Blockchain Visualization
Run python3 generate_blockchain_graph.py to create block chain graph.  
Outputs will be stored in Output/Blockchain folder  


