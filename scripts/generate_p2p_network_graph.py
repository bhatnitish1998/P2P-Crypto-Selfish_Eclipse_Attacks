import matplotlib.pyplot as plt
import networkx as nx
import os
import matplotlib.patches as mpatches

def initialize_path():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir) 

    # Create paths relative to the script directory
    input_filepath = os.path.join(project_root, "Output/Temp_files", "network.txt")
    output_folder = os.path.join(project_root, "Output/Network")
    return input_filepath,output_folder

def load_network_graph_from_file(filepath):
    graph = nx.Graph()
    with open(filepath, 'r') as f:
        for line in f:
            node_data = list(map(int, line.strip().split()))
            source_node = node_data[0]
            target_node = node_data[1]
            graph.add_edge(source_node, target_node)
    return graph

def plot_network_graph(graph, directory):
    layout = nx.spring_layout(graph)
    num_nodes = len(graph.nodes)  
    
    # Scale size based on nodes
    size = max(10, num_nodes * 1.05)  
    # print(size)
    plt.figure(figsize=(size, size))
    nx.draw(graph, layout,edge_color='black',node_color='lightgreen',node_size=2200,font_size=24,with_labels=True,)
    
    # create targte directory if not exists
    os.makedirs(directory, exist_ok=True)
    
    legend_patch = [
        mpatches.Patch(color='lightgreen', label='Peer / Node')
    ]   
    # Save the figure as a PNG file within the given directory
    plt.legend(handles=legend_patch, loc='upper left' ,fontsize=24)
    output_file = os.path.join(directory, 'network_graph.png')
    plt.savefig(output_file, format='png')
    plt.close()

if __name__ == "__main__":
    input_filepath,output_folder=initialize_path()
    network_graph = load_network_graph_from_file(input_filepath)
    plot_network_graph(network_graph, output_folder)

