import matplotlib.pyplot as plt
import networkx as nx
import os
import glob
import matplotlib.patches as mpatches
import matplotlib.animation as animation
from collections import defaultdict, deque

def initialize_path():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)

    input_folder = os.path.join(project_root, "Output/Node_Files")
    input_files = glob.glob(os.path.join(input_folder, "Node_0.txt"))
    output_folder = os.path.join(project_root, "Output/Blockchains")
    return input_files, output_folder

def load_blockchain_graph_from_file(filepath):
    edges = []
    node_mining_data = {}

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
    return edges, node_mining_data

def assign_positions(edges, root=0, dx=12.0, dy=18):
    graph = nx.DiGraph()
    graph.add_edges_from(edges)

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

def animate_blockchain(edges, node_mining_data, output_folder, output_filename):
    full_graph = nx.DiGraph()
    full_graph.add_edges_from(edges)
    all_nodes = list(full_graph.nodes())
    
    fig, ax = plt.subplots(figsize=(40, 8))
    ax.set_facecolor('black')
    
    # Remove padding around the figure
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)

    # Remove extra margins from the figure
    ax.set_xticks([])
    ax.set_yticks([])
    ax.set_frame_on(False)

    fixed_positions = assign_positions(edges, root=0)

    def get_node_colors(nodes):
        return [
            '#FF8C00' if node_mining_data.get(node, 0) == 1 else
            ('#00CED1' if node == 0 or full_graph.degree(node) >= 2 else '#32CD32')
            for node in nodes
        ]

    def update(num):
        ax.clear()
        ax.set_facecolor('black')

        current_edges = []
        if num == 0:
            current_nodes = set([])
        elif num == 1:
            current_nodes = set([edges[0][0]])
        else:
            current_edges = edges[:num-1]
            current_nodes = set(n for edge in current_edges for n in edge)

        frame_graph = nx.DiGraph()
        frame_graph.add_nodes_from(all_nodes)
        frame_graph.add_edges_from(current_edges)

        node_colors = get_node_colors(all_nodes)

        nx.draw_networkx_nodes(
            frame_graph.reverse(),
            pos=fixed_positions,
            nodelist=all_nodes,
            node_color=node_colors,
            node_size=500,
            node_shape='s',
            alpha=[0.8 if node in current_nodes else 0.0 for node in all_nodes],
            
            # linewidths=2,
            # edgecolors='black'
        )

        if num > 0:
            nx.draw_networkx_edges(
                frame_graph.reverse(),
                pos=fixed_positions,
                edgelist=current_edges,
                edge_color='black',
                width=2,
                arrowsize=20,
                arrowstyle='-|>'
            )

        nx.draw_networkx_labels(
            frame_graph.reverse(),
            pos=fixed_positions,
            labels={n: str(n) for n in current_nodes},
            font_size=14,
            font_color='black'
        )

        legend_patches = [
            mpatches.Patch(color='#FF8C00', label='Block Mined by this Node'),
            mpatches.Patch(color='#00CED1', label='Non-Leaf Block'),
            mpatches.Patch(color='#32CD32', label='Leaf Block')
        ]
        ax.legend(handles=legend_patches, loc='upper left', fontsize=14, 
                  facecolor='black', edgecolor='black', labelcolor='white')

        all_x = [p[0] for p in fixed_positions.values()]
        all_y = [p[1] for p in fixed_positions.values()]
        ax.set_xlim(min(all_x) - 5, max(all_x) + 5)
        ax.set_ylim(min(all_y) - 5, max(all_y) + 5)

    ani = animation.FuncAnimation(
        fig, 
        update, 
        frames=len(edges) + 1, 
        interval=10, 
        repeat=False,
    )

    # Ensure tight layout before saving
    plt.tight_layout(pad=0)

    os.makedirs(output_folder, exist_ok=True)
    video_path = os.path.join(output_folder, output_filename.replace(".txt", ".mp4"))
    ani.save(video_path, writer='ffmpeg', fps=2, dpi=100)
    plt.close()
    print(f"Saved animation: {video_path}")

if __name__ == "__main__":
    input_files, output_folder = initialize_path()
    for input_filepath in input_files:
        edges, node_mining_data = load_blockchain_graph_from_file(input_filepath)
        output_filename = os.path.basename(input_filepath)
        animate_blockchain(edges, node_mining_data, output_folder, output_filename)
