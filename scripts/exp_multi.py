import os
import re
import csv
import numpy as np
import time
import shutil
import subprocess
import itertools
import threading
from tqdm import tqdm
import math

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
    """Extracts the ringmaster node ID from the CSV file."""
    with open(node_properties_filepath, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if int(row['ringmaster']) == 1:
                return int(row['node_id'])
    print("Error: No ringmaster node found!")
    return None

def run_experiment(params, results_file_path, experiment_subfolder):
    eclipse_attack, number_of_nodes, percent_malicious, mean_tx_time, block_time, timeout = params
    project_root = os.getcwd()
    exp_id = f"{eclipse_attack}_{number_of_nodes}_{percent_malicious}_{mean_tx_time}_{block_time}_{timeout}"
    output_dir_name = f"Output_{exp_id}"
    output_folder = os.path.join(experiment_subfolder, output_dir_name)
    os.makedirs(output_folder, exist_ok=True)
    # temp_folder = os.path.join(experiment_subfolder, f"temp_{percent_malicious}_{timeout}_{eclipse_attack}")
    # os.makedirs(temp_folder, exist_ok=True)
    
    main_executable = os.path.join(project_root, "main.exe" if os.name == 'nt' else "main")
    cmd = [main_executable, str(number_of_nodes), str(percent_malicious), str(mean_tx_time), str(block_time), str(timeout), output_folder]
    if eclipse_attack:
        cmd.append("--eclipse")
    print(f"\nExecuting: {' '.join(cmd)}")
    process = subprocess.run(cmd, capture_output=True, text=True)
    
    time.sleep(3)
    
    node_properties_filepath = os.path.join(output_folder, "Temp_files", "all_node_details.csv")
    input_folder = os.path.join(output_folder, "Node_Files")
    ringmaster_node = get_ringmaster_node(node_properties_filepath)
    if ringmaster_node is None:
        return
    
    input_file = os.path.join(input_folder, f"Node_{ringmaster_node}.txt")
    if os.path.exists(input_file):
        node_data = parse_node_file(input_file)
        ratio_1, ratio_2 = compute_ratios(node_data)
    else:
        ratio_1, ratio_2 = 'Nan', 'Nan'
    
    with open(results_file_path, "a", newline='') as results_file:
        writer = csv.writer(results_file)
        writer.writerow([number_of_nodes, percent_malicious, mean_tx_time, block_time, timeout, eclipse_attack, round(ratio_1, 4), round(ratio_2, 4)])

def main():
    """ Run as: python scripts/exp.py and relax!"""
    
    """------------------- Set params here ------------------------"""
    number_of_nodes_list                = [10]
    percent_malicious_list              = [20, 30, 50, 60]  
    mean_transaction_inter_arrival_list = [1000]
    block_inter_arrival_list            = [100]
    timeout_time_list                   = [500, 1000, 1500]
    eclipse_attack_list                 = [False, True]
    """--------------------------------------------------------------"""
    
    project_root = os.getcwd()
    experiment_root = os.path.join(project_root, "experiments")
    os.makedirs(experiment_root, exist_ok=True)
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    experiment_subfolder = os.path.join(experiment_root, timestamp)
    os.makedirs(experiment_subfolder, exist_ok=True)
    
    results_file_path = os.path.join(experiment_subfolder, "results.csv")
    
    with open(results_file_path, "w", newline='') as results_file:
        writer = csv.writer(results_file)
        writer.writerow(["number_of_nodes", "percent_malicious", "mean_transaction_inter_arrival", "block_inter_arrival", "timeout", "is_eclipse", "ratio_1", "ratio_2"])
    
    combinations_param_list = [
        eclipse_attack_list,
        number_of_nodes_list,
        percent_malicious_list,
        mean_transaction_inter_arrival_list,
        block_inter_arrival_list,
        timeout_time_list,
    ]
    
    total_combinations = math.prod(map(len, combinations_param_list))
    exp_params = list(itertools.product(*combinations_param_list))
    print(f"\nTotal no. of experiments: {total_combinations}\n")
    
    threads = []
    for params in tqdm(exp_params, desc="Running experiments", unit="experiment", total=total_combinations):
        thread = threading.Thread(target=run_experiment, args=(params, results_file_path, experiment_subfolder))
        thread.start()
        threads.append(thread)
    
    for thread in threads:
        thread.join()
    
    print(f"Results saved to {results_file_path}")
    
    plot_script = os.path.join(os.getcwd(), "scripts", "plot.py")
    subprocess.run(['python3', plot_script, results_file_path])

if __name__ == "__main__":
    main()
