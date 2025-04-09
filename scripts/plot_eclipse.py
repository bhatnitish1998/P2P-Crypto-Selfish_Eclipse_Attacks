import pandas as pd
import matplotlib.pyplot as plt
import argparse
import os

"""
Usage: python scripts/plot.py <csv_file_path> --fixed_timeout <timeout_value> --fixed_percent_malicious <percent_value>
"""

parser = argparse.ArgumentParser(description="Plot data from a CSV file.")
parser.add_argument("csv_file_path", type=str, help="Path to the CSV file.")
parser.add_argument("--fixed_timeout", type=int, required=True, help="Timeout value to filter data.")
parser.add_argument("--fixed_percent_malicious", type=int, required=True, help="Percent malicious to filter data.")

args = parser.parse_args()
csv_dir = os.path.dirname(args.csv_file_path)

data = pd.read_csv(args.csv_file_path)
data = data.sort_values(by=['percent_malicious', 'timeout'])

is_eclipse_values = data['is_eclipse'].unique()

data_timeout_fixed = data[data['timeout'] == args.fixed_timeout]

data_malicious_fixed = data[data['percent_malicious'] == args.fixed_percent_malicious]

if data_timeout_fixed.empty or data_malicious_fixed.empty:
    print("No data found for the given timeout or percent_malicious values.")
    exit()

fig, axes = plt.subplots(1, 2, figsize=(14, 5))

# First subplot: Ratio vs Percent Malicious (fixed timeout)
ax1 = axes[0]
for is_eclipse in is_eclipse_values:
    subset = data_timeout_fixed[data_timeout_fixed['is_eclipse'] == is_eclipse]
    ax1.plot(subset['percent_malicious'], subset['ratio_1'], marker='o', linestyle='-', label=f"Ratio 1 (Eclipse {is_eclipse})")

ax1.set_xlabel("Malicious Percentages")
ax1.set_ylabel("Ratios")
ax1.set_title(f"Ratio 1 vs Malicious Percent (Timeout={args.fixed_timeout})")
ax1.legend()
ax1.grid(True)

# Second subplot: Ratio vs Timeout (fixed percent_malicious)
ax2 = axes[1]
for is_eclipse in is_eclipse_values:
    subset = data_malicious_fixed[data_malicious_fixed['is_eclipse'] == is_eclipse]
    ax2.plot(subset['timeout'], subset['ratio_1'], marker='o', linestyle='-', label=f"Ratio 1 (Eclipse {is_eclipse})")

ax2.set_xlabel("Timeout")
ax2.set_ylabel("Ratios")
ax2.set_title(f"Ratio 1 vs Timeout (Malicious {args.fixed_percent_malicious}%)")
ax2.legend()
ax2.grid(True)

# Set main title
# plt.suptitle(f"Comparison of Ratio 1 for Different Conditions")

plt.tight_layout(rect=[0, 0, 1, 0.95])

# Save the plot
plot_filename = f"comparison_plot_t{args.fixed_timeout}_m{args.fixed_percent_malicious}.png"
plot_path = os.path.join(csv_dir, plot_filename)
plt.savefig(plot_path)
plt.close()

print(f"Plot saved at: {plot_path}")
