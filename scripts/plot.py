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
# Extract the required columns from the CSV
malicious_percentages = data['percent_malicious']
timeouts = data['timeout']
ratio_1 = data['ratio_1']
ratio_2 = data['ratio_2']

# Get unique timeout values
timeouts_unique = data['timeout'].unique()

# Determine appropriate y-axis limits for each ratio
ratio_1_min, ratio_1_max = ratio_1.min(), ratio_1.max()
ratio_2_min, ratio_2_max = ratio_2.min(), ratio_2.max()

# Plot settings
fig, axes = plt.subplots(1, 2, figsize=(14, 5))

# Plot for ratio_1
ax1 = axes[0]
for timeout in timeouts_unique:
    indices = data['timeout'] == timeout
    ax1.plot(
        malicious_percentages[indices],
        ratio_1[indices],
        marker='o', linestyle='-', label=f"Timeout {timeout}"
    )
ax1.set_xlabel("Malicious Percentages")
ax1.set_ylabel("Ratio 1")
ax1.set_title("Ratio 1 vs Malicious Percentages")
ax1.set_ylim(ratio_1_min - 0.01, ratio_1_max + 0.01)  # Adjust y-axis range
ax1.legend()
ax1.grid(True)

# Plot for ratio_2
ax2 = axes[1]
for timeout in timeouts_unique:
    indices = data['timeout'] == timeout
    ax2.plot(
        malicious_percentages[indices],
        ratio_2[indices],
        marker='s', linestyle='--', label=f"Timeout {timeout}"
    )
ax2.set_xlabel("Malicious Percentages")
ax2.set_ylabel("Ratio 2")
ax2.set_title("Ratio 2 vs Malicious Percentages")
ax2.set_ylim(ratio_2_min - 0.1, ratio_2_max + 0.1)  # Adjust y-axis range
ax2.legend()
ax2.grid(True)

plt.suptitle("Ratio 1 and Ratio 2 for Different Timeout Values with Adjusted Y-axis")
plt.tight_layout(rect=[0, 0, 1, 0.95])
plot_path = os.path.join(csv_dir, "plot.png")
plt.savefig(plot_path)
plt.close()

print(f"Plot saved at: {plot_path}")
