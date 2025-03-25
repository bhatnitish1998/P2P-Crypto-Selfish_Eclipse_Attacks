import matplotlib.pyplot as plt
import networkx as nx
import os
import matplotlib.patches as mpatches
import csv

def initialize_path():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir) 

    # Define file paths
    input_filepath = os.path.join(project_root, "Output/Temp_files", "network_common.txt")
    # new addition
    input_filepath_malicious = os.path.join(project_root, "Output/Temp_files", "network_malicious.txt")
    node_properties_filepath = os.path.join(project_root, "Output/Temp_files", "all_node_details.csv")
    
    output_folder = os.path.join(project_root, "Output/Network")

    return input_filepath,input_filepath_malicious,node_properties_filepath, output_folder


def load_network_graph_from_file(filepath, graph=None):
    if graph is None:
        graph = nx.Graph()  # Create a new graph if not provided

    with open(filepath, 'r') as f:
        for line in f:
            node_data = list(map(int, line.strip().split()))
            source_node = node_data[0]
            target_node = node_data[1]

            # Check if edge already exists before adding
            if not graph.has_edge(source_node, target_node):
                graph.add_edge(source_node, target_node)
    
    return graph


def load_node_properties(filepath):
    node_colors = {}
    with open(filepath, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            node_id = int(row['node_id'])
            malicious = int(row['malicious'])
            ringmaster = int(row['ringmaster'])

            if ringmaster == 1:
                node_colors[node_id] = 'red'  # Ringmaster (highest priority)
            elif malicious == 1:
                node_colors[node_id] = 'orange'  # Malicious
            else:
                node_colors[node_id] = 'lightgreen'  # Honest
    
    return node_colors

def plot_network_graph(graph, node_colors, directory,filename):
    layout = nx.spring_layout(graph)
    num_nodes = len(graph.nodes)

    # Adjust figure size dynamically
    size = max(10, num_nodes * 1.5)
    plt.figure(figsize=(size, size))

    # Assign colors to nodes
    node_color_list = [node_colors.get(node, 'lightgreen') for node in graph.nodes]

    nx.draw(
        graph, layout, edge_color='black', node_color=node_color_list,
        node_size=2200, font_size=24, with_labels=True
    )

    # Ensure the output directory exists
    os.makedirs(directory, exist_ok=True)

    # Legend
    legend_patches = [
        mpatches.Patch(color='lightgreen', label='Honest Node'),
        mpatches.Patch(color='orange', label='Malicious Node'),
        mpatches.Patch(color='red', label='Ringmaster Node')
    ]
    plt.legend(handles=legend_patches, loc='upper left', fontsize=22)

    # Save the figure
    output_file = os.path.join(directory, filename)
    plt.savefig(output_file, format='png')
    plt.close()

if __name__ == "__main__":
    input_filepath,input_filepath_malicious, node_properties_filepath, output_folder = initialize_path()
     # Load common and malicious edges in same graph
    network_graph = load_network_graph_from_file(input_filepath)

    malicious_graph = load_network_graph_from_file(input_filepath_malicious)

    node_colors = load_node_properties(node_properties_filepath)

    plot_network_graph(network_graph, node_colors, output_folder, "common_network_graph.png")
    plot_network_graph(malicious_graph, node_colors, output_folder, "overlay_network_graph.png")
