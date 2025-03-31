import pandas as pd
import matplotlib.pyplot as plt
import argparse
import os

"""
Usage: python scripts/plot.py <csv_file_path>
"""

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Plot data from a CSV file.")
parser.add_argument("csv_file_path", type=str, help="Path to the CSV file.")
args = parser.parse_args()
csv_dir = os.path.dirname(args.csv_file_path)

data = pd.read_csv(args.csv_file_path)
data = data.sort_values(by=['percent_malicious', 'timeout'])

# Get unique values for is_eclipse
is_eclipse_values = data['is_eclipse'].unique()

for is_eclipse in is_eclipse_values:
    subset = data[data['is_eclipse'] == is_eclipse]
    malicious_percentages = subset['percent_malicious']
    timeouts = subset['timeout']
    ratio_1 = subset['ratio_1']
    ratio_2 = subset['ratio_2']
    timeouts_unique = subset['timeout'].unique()

    ratio_1_min, ratio_1_max = ratio_1.min(), ratio_1.max()
    ratio_2_min, ratio_2_max = ratio_2.min(), ratio_2.max()

    fig, axes = plt.subplots(1, 2, figsize=(14, 5))

    # Plot for ratio_1
    ax1 = axes[0]
    for timeout in timeouts_unique:
        indices = subset['timeout'] == timeout
        ax1.plot(
            malicious_percentages[indices],
            ratio_1[indices],
            marker='o', linestyle='-', label=f"Timeout {timeout}"
        )
    ax1.set_xlabel("Malicious Percentages")
    ax1.set_ylabel("Ratio 1")
    ax1.set_title(f"Ratio 1 vs Malicious Percentages (is_eclipse={is_eclipse})")
    ax1.set_ylim(ratio_1_min - 0.01, ratio_1_max + 0.01)
    ax1.legend()
    ax1.grid(True)

    # Plot for ratio_2
    ax2 = axes[1]
    for timeout in timeouts_unique:
        indices = subset['timeout'] == timeout
        ax2.plot(
            malicious_percentages[indices],
            ratio_2[indices],
            marker='s', linestyle='--', label=f"Timeout {timeout}"
        )
    ax2.set_xlabel("Malicious Percentages")
    ax2.set_ylabel("Ratio 2")
    ax2.set_title(f"Ratio 2 vs Malicious Percentages (is_eclipse={is_eclipse})")
    ax2.set_ylim(ratio_2_min - 0.1, ratio_2_max + 0.1)
    ax2.legend()
    ax2.grid(True)

    plt.suptitle(f"Ratio 1 and Ratio 2 for Different Timeout Values (is_eclipse={is_eclipse})")
    plt.tight_layout(rect=[0, 0, 1, 0.95])

    plot_filename = f"plot_is_eclipse_{is_eclipse}.png"
    plot_path = os.path.join(csv_dir, plot_filename)
    plt.savefig(plot_path)
    plt.close()

    print(f"Plot saved at: {plot_path}")