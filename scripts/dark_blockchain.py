import matplotlib.pyplot as plt
import networkx as nx
from collections import defaultdict, deque
import os
import glob
import matplotlib.patches as mpatches

def initialize_path():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir) 

    input_folder = os.path.join(project_root, "Output/Node_Files")
    input_files = glob.glob(os.path.join(input_folder, "Node_*.txt"))
    output_folder = os.path.join(project_root, "Output/Blockchains")
    return input_files, output_folder

def load_blockchain_graph_from_file(filepath):
    graph = nx.DiGraph()
    node_mining_data = {}
    edges = []

    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    start_index = None
    for i, line in enumerate(lines):
        if line.strip() == "Blockchain:":
            start_index = i + 2  
            break

    if start_index is None:
        raise ValueError("Blockchain section not found in the file")

    for line in lines[start_index:]:
        parts = line.strip().split(',')
        if len(parts) < 6:
            continue  
        block_id = int(parts[0].strip())
        parent_id = int(parts[1].strip())
        mined_by = int(parts[5].strip())
        if parent_id != -1:
            edges.append((parent_id, block_id))

        node_mining_data[block_id] = mined_by

    edges.sort()
    graph.add_edges_from(edges)
    
    return graph, node_mining_data

def assign_positions(graph, root, dx=4.0, dy=1.0):
    pos = {}  
    level_nodes = defaultdict(list)
    queue = deque([(root, 0)])  
    visited = set()

    while queue:
        node, level = queue.popleft()
        if node in visited:
            continue
        visited.add(node)
        level_nodes[level].append(node)

        for neighbor in graph.successors(node):
            if neighbor not in visited:
                queue.append((neighbor, level + 1))

    for level, nodes in level_nodes.items():
        num_nodes = len(nodes)
        start_y = -((num_nodes - 1) * dy) / 2  
        for i, node in enumerate(nodes):
            pos[node] = (level * dx, start_y + i * dy)

    return pos

def plot_blockchain_graph(graph, node_colors, directory, output_filename):
    pos = assign_positions(graph, root=0, dx=12.0, dy=18)
    num_nodes = len(graph.nodes) 

    legend_patches = [
        mpatches.Patch(color='#FF8C00', label='Block Mined by this Node'),  
        mpatches.Patch(color='#00CED1', label='Non-Leaf Block'),  
        mpatches.Patch(color='#32CD32', label='Leaf Block')  
    ]   

    width = max(10, num_nodes * 2)  
    plt.figure(figsize=(width, 12), facecolor='black')  
    # ax = plt.gca()
    # ax.set_facecolor('black')  

    nx.draw(
        graph.reverse(),
        pos=pos,
        edge_color='white',  
        node_color=node_colors,
        node_size=3000,
        with_labels=True,
        font_size=20,
        font_color='black',  
        node_shape='s',
        linewidths=2,  
        edgecolors='white',
        width=2,
        arrowsize=20 
    )

    # plt.title("Blockchain Visualization", color='white', fontsize=24)
    plt.legend(handles=legend_patches, loc='upper left', fontsize=24, facecolor='white', edgecolor='white')

    os.makedirs(directory, exist_ok=True)
    output_file = os.path.join(directory, output_filename)
    plt.savefig(output_file, format='png', facecolor='black', dpi=100)
    plt.close()

if __name__ == "__main__":
    input_files, output_folder = initialize_path()
    for idx, input_filepath in enumerate(input_files):
        output_filename = os.path.splitext(os.path.basename(input_filepath))[0] + ".png"
        blockchain_graph, node_mining_data = load_blockchain_graph_from_file(input_filepath)

        node_colors = [
            '#FF8C00' if node_mining_data.get(node, 0) == 1 else  
            ('#00CED1' if node == 0 or blockchain_graph.degree(node) >= 2 else '#32CD32')  
            for node in blockchain_graph.nodes()
        ]

        plot_blockchain_graph(blockchain_graph, node_colors, output_folder, output_filename)
        
        if idx == 0:
            print("Created Blockchain: ", end=" ", flush=True)
        print(output_filename, end=" ", flush=True)
