import os
import re
import csv
import numpy as np
import time
import shutil
import subprocess
import itertools
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

def main():
    """ Run as: python scripts/exp.py and relax!"""

    """------------------- Set params here ------------------------"""
    # Define parameter ranges
    number_of_nodes_list                = [50]
    percent_malicious_list              = [5,10,20,40,60]  
    mean_transaction_inter_arrival_list = [50]
    block_inter_arrival_list            = [600]
    timeout_time_list                   = [50,500,1000,1500,3000]
    eclipse_attack_list                 = [False]
    """--------------------------------------------------------------"""

    project_root = os.getcwd()
    output_folder = os.path.join(project_root, "Output")
    node_properties_filepath = os.path.join(output_folder, "Temp_files", "all_node_details.csv")
    input_folder = os.path.join(output_folder, "Node_Files")
    
    # Experiment folder setup
    experiment_root = os.path.join(project_root, "experiments")
    os.makedirs(experiment_root, exist_ok=True)
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    experiment_subfolder = os.path.join(experiment_root, timestamp)
    os.makedirs(experiment_subfolder, exist_ok=True)
    
    results_file_path = os.path.join(experiment_subfolder, "results.csv")
    
    # Write CSV header
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
        eclipse_attack_list,
    ]
        
    total_combinations = math.prod(map(len, combinations_param_list))

    exp_params = itertools.product(*combinations_param_list)

    print(f"\nTotal no. of experiments: {total_combinations}\n")

    for params in tqdm(exp_params, desc="Running experiments", unit="experiment", total=total_combinations):
        
        number_of_nodes, percent_malicious, mean_tx_time, block_time, timeout, eclipse_attack = params
        
        # Delete Output and Temp folder
        if os.path.exists(output_folder):
            shutil.rmtree(output_folder)
        temp_folder = os.path.join(project_root, "Temp_files")
        if os.path.exists(temp_folder):
            shutil.rmtree(temp_folder)
        # print("\nOld Output and temp folder deleted")

        # Run main executable
        main_executable = os.path.join(project_root, "main.exe" if os.name == 'nt' else "main")
        cmd = [main_executable, str(number_of_nodes), str(percent_malicious), str(mean_tx_time), str(block_time), str(timeout)]
        if eclipse_attack:
            cmd.append("--eclipse")
        # cmd.append(f" > {experiment_subfolder}/exp_stdout.log")
        # print(f"\nExecuting: {' '.join(cmd)}")
        process = subprocess.run(cmd, capture_output=True, text=True)
        # print("stdout:\n", process.stdout)
        # print("stderr:\n", process.stderr)
        
        time.sleep(3)

        # Parse results
        ringmaster_node = get_ringmaster_node(node_properties_filepath)
        if ringmaster_node is None:
            continue
        

        input_file = os.path.join(input_folder, f"Node_{ringmaster_node}.txt")
        if os.path.exists(input_file):
            node_data = parse_node_file(input_file)
            ratio_1, ratio_2 = compute_ratios(node_data)
        else:
            ratio_1, ratio_2 = 'Nan', 'Nan'
        
        # Save results to CSV
        with open(results_file_path, "a", newline='') as results_file:
            writer = csv.writer(results_file)
            writer.writerow([number_of_nodes, percent_malicious, mean_tx_time, block_time, timeout, eclipse_attack, round(ratio_1, 4), round(ratio_2, 4)])
    
    print(f"Results saved to {results_file_path}")

    plot_script = './scripts/plot.py'
    subprocess.run(['python3', plot_script, results_file_path])
    # print(f'Plots saved at: {results_file_path}/plot.py')
    
if __name__ == "__main__":
    main()
