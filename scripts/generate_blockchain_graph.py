import matplotlib.pyplot as plt
import networkx as nx
from collections import defaultdict,deque
import os
import glob
import matplotlib.patches as mpatches
import matplotlib

matplotlib.use('agg')

def initialize_path():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir) 

    # Create paths relative to the script directory
    input_folder = os.path.join(project_root, "output/Node_Files")
    # create graph for each node
    input_files = glob.glob(os.path.join(input_folder, "Node_*.txt"))
    output_folder = os.path.join(project_root, "output/Blockchains")
    return input_files,output_folder

def load_blockchain_graph_from_file(filepath):
    graph = nx.DiGraph()
    node_mining_data = {}
    # print(graph)
    edges = []  #
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    # Locate Blockchain section
    start_index = None
    for i, line in enumerate(lines):
        if line.strip() == "Blockchain:":
            # node creating data present at start_index +2 (skipping headers)
            start_index = i + 2  
            break

    if start_index is None:
        raise ValueError("Blockchain section not found in the file")

    # Process the blockchain node data
    for line in lines[start_index:]:
        parts = line.strip().split(',')
        if len(parts) < 6:
            continue  # Skip invalid lines
        block_id = int(parts[0].strip())
        parent_id = int(parts[1].strip())
        mined_by = int(parts[5].strip())
        if parent_id != -1:
            edges.append((parent_id, block_id))

        node_mining_data[block_id] = mined_by
    edges.sort()
    graph.add_edges_from(edges)
    # print(filepath,node_mining_data)
    
    return graph,node_mining_data

def assign_positions(graph, root, dx=4.0, dy=1.0):
    """Assigns x, y positions using BFS."""
    pos = {}  # Store node positions
    level_nodes = defaultdict(list)  # Nodes at each level
    queue = deque([(root, 0)])  # (node, level)
    visited = set()

    # BFS to determine levels
    while queue:
        node, level = queue.popleft()
        if node in visited:
            continue
        visited.add(node)
        level_nodes[level].append(node)

        for neighbor in graph.successors(node):
            if neighbor not in visited:
                queue.append((neighbor, level + 1))

    # Assign positions
    for level, nodes in level_nodes.items():
        num_nodes = len(nodes)
        start_y = -((num_nodes - 1) * dy) / 2  # Center nodes vertically
        for i, node in enumerate(nodes):
            pos[node] = (level * dx, start_y + i * dy)

    return pos

def plot_blockchain_graph(graph,node_colors,directory,output_filename):
    """
    plots the blocks chain graph based on position.
    """
    pos = assign_positions(graph, root=0, dx=12.0, dy=18)
    num_nodes = len(graph.nodes) 
    
    legend_patches = [
        mpatches.Patch(color='#FFA07A', label='Block Mined by this Node'),
        mpatches.Patch(color='lightblue', label='Non Leaf Block'),
        mpatches.Patch(color='lightgreen', label='Leaf Block')
    ]   
    # Scale width based on #nodes
    width = max(10, num_nodes * 2)  
    plt.figure(figsize=(width, 12))  
    nx.draw(graph.reverse(),pos=pos,edge_color='black',node_color=node_colors,node_size=3000,with_labels=True,font_size=18,node_shape='s')

    plt.title("block chain visualization")
    plt.legend(handles=legend_patches, loc='upper left' ,fontsize=24)
    # create targte directory if not exists
    os.makedirs(directory, exist_ok=True)
    
    # Save the figure as a PNG file within the given directory
    output_file = os.path.join(directory, output_filename)
    plt.savefig(output_file, format='png')
    plt.close()

if __name__ == "__main__":
    input_files,output_folder=initialize_path()
    for input_filepath in input_files:
        output_filename = os.path.splitext(os.path.basename(input_filepath))[0] + ".png"
        # print(output_filename)
        blockchain_graph,node_mining_data = load_blockchain_graph_from_file(input_filepath)
        # print("Edges:", list(blockchain_graph)) 
        # Assign colors based on mining node and degree
        node_colors = [
            '#FFA07A' if node_mining_data.get(node, 0) == 1 else 
            ('lightblue' if node == 0 or blockchain_graph.degree(node) >= 2 else 'lightgreen') 
            for node in blockchain_graph.nodes()
        ]

        plot_blockchain_graph(blockchain_graph,node_colors,output_folder,output_filename)
        print("created Blockchain",output_filename)

