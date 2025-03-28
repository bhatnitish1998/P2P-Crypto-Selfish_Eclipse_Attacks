import os
import re
import csv
import numpy as np
import time
import sys
import shutil
import subprocess

# example run: python3 scripts/stats.py 10 10 40 10 1000 50 1000 2000 200 --eclipse

def parse_node_file(file_path):
    """Parses the node log file and extracts relevant blockchain statistics."""
    data = {
        "blocks_mined_total": 0.0,
        "blocks_mined_in_longest_chain": 0.0,
        "Fraction_of_blocks_mined_in_longest_chain": 0.0,
    }
    
    with open(file_path, 'r') as f:
        content = f.readlines()
    
    for line in content:
        if line.startswith("Blocks mined: "):
            data["blocks_mined_total"] = float(line.split(": ")[1])
        elif line.startswith("Blocks mined in longest chain: "):
            data["blocks_mined_in_longest_chain"] = float(line.split(": ")[1])
        elif line.startswith("Fraction of blocks mined in longest chain: "):
            data["Fraction_of_blocks_mined_in_longest_chain"] = float(line.split(": ")[1])
        elif line.strip() == "Blockchain:":
            break
    
    return data

def compute_ratios(node_data):
    """Computes the required ratios."""

    ratio_1 = node_data["Fraction_of_blocks_mined_in_longest_chain"]
    ratio_2 = (node_data["blocks_mined_in_longest_chain"] / node_data["blocks_mined_total"]) if node_data["blocks_mined_total"] > 0 else 0
    return ratio_1, ratio_2

def get_ringmaster_node(node_properties_filepath):
    ringmaster_node = None
    # extract ringmaster node 
    with open(node_properties_filepath, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            node_id = int(row['node_id'])
            if int(row['ringmaster']) == 1:
                ringmaster_node = node_id
    
    if ringmaster_node is None:
        print("Error: No ringmaster node found!")
        return
    return ringmaster_node

def main():

    if len(sys.argv) < 10:
        print("Usage: python3 scripts/stats.py <number_of_nodes> <percent_malicious_start> <percent_malicious_end> <<percent_malicious_step_size> <mean_transaction_inter_arrival_time> <block_inter_arrival_time> <timeout time stat> <timeout time end> <timeout time step size>")
        return
    
    number_of_nodes = int(sys.argv[1])
    percent_malicious_nodes_start = int(sys.argv[2])
    percent_malicious_nodes_end = int(sys.argv[3])
    percent_malicious_nodes_step_size = int(sys.argv[4])
    mean_transaction_inter_arrival_time = int(sys.argv[5])
    block_inter_arrival_time = int(sys.argv[6])
    timer_timeout_time_start = int(sys.argv[7])
    timer_timeout_time_end = int(sys.argv[8])
    timer_timeout_time_step_size = int(sys.argv[9])
    eclipse_attack = "--eclipse" in sys.argv

    print("eclipse",eclipse_attack)

    project_root = os.getcwd()
    output_folder = os.path.join(project_root, "Output")
    node_properties_filepath = os.path.join(project_root, "Output/Temp_files", "all_node_details.csv")
    input_folder = os.path.join(project_root, "Output/Node_Files")
    
    # experiment folder
    experiment_root = os.path.join(project_root, "experiments")
    os.makedirs(experiment_root, exist_ok=True)
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    experiment_subfolder = os.path.join(experiment_root, timestamp)
    os.makedirs(experiment_subfolder, exist_ok=True)


    # print(input_file)
    malicious_percentages = list(range(percent_malicious_nodes_start, percent_malicious_nodes_end + 1, percent_malicious_nodes_step_size))
    timeout_times = list(range(timer_timeout_time_start, timer_timeout_time_end + 1, timer_timeout_time_step_size))
    
    results = {}
    
    results_file_path = os.path.join(experiment_subfolder, "results.txt")
    with open(results_file_path, "w") as results_file:
        results_file.write("malicious_percentages,timeout,ratio_1,ratio_2,isEclipse\n")

    for percent in malicious_percentages:
        for Tt in timeout_times:
            
            # Delete Output folder
            output_folder = os.path.join(project_root, "Output")
            if os.path.exists(output_folder):
                shutil.rmtree(output_folder)
            temp_folder = os.path.join(project_root, "Temp_files")
            if os.path.exists(temp_folder):
                shutil.rmtree(temp_folder)
            print("old Output and temp folder deleted")

            # Run main executable
            print(f"Running experiment with: number_of_nodes={number_of_nodes}, percent_malicious={percent}, mean_transaction_inter_arrival_time={mean_transaction_inter_arrival_time}, block_inter_arrival_time={block_inter_arrival_time}, timeout={Tt}, eclipse_attack={eclipse_attack}")
            main_executable = os.path.join(project_root, "main")
            cmd = [main_executable, str(number_of_nodes), str(percent), str(mean_transaction_inter_arrival_time), str(block_inter_arrival_time), str(Tt)]
            if eclipse_attack:
                cmd.append("--eclipse")
            print(f"Executing main for % {percent} and Timeout {Tt} and Eclipse_attack {eclipse_attack}: {' '.join(cmd)}")

            # Run the command and capture stdout & stderr
            process = subprocess.run(cmd, capture_output=True, text=True)

            # Print logs
            print("stdout:\n", process.stdout)
            print("stderr:\n", process.stderr)

            # now 
            # time.sleep(1000)
            print("main execution completed")
            ringmaster_node =get_ringmaster_node(node_properties_filepath)
            input_file = [os.path.join(input_folder, f"Node_{ringmaster_node}.txt")]


            if os.path.exists(input_folder):
                node_data = parse_node_file(input_file[0])
                # print(node_data)
            
            ratio_1, ratio_2 = compute_ratios(node_data)
            results[(percent, Tt)] = (ratio_1, ratio_2)

            with open(results_file_path, "a") as results_file:
                results_file.write(f"{percent},{Tt},{ratio_1:.4f},{ratio_2:.4f},{eclipse_attack}\n")
            
            # print(f"Malicious %: {percent}, Timeout: {Tt}, Ratio 1: {ratio_1:.4f}, Ratio 2: {ratio_2:.4f}")           
    print(f"Output available  in folder {results_file_path}")

if __name__ == "__main__":
    main()
