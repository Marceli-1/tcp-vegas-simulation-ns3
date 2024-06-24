import re
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

# Read the content of the results.txt file
with open('results.txt', 'r') as file:
    content = file.read()

# Define the pattern to match the entries
pattern = re.compile(
    r'Alpha: (\d+) Beta: (\d+) Gamma: (\d+) FileSize: (\d+)MB Average Throughput: ([\d\.]+) Mbps'
)

# Extract the data using the pattern
matches = pattern.findall(content)

# Create a DataFrame from the extracted data
df = pd.DataFrame(matches, columns=[
    'Alpha', 'Beta', 'Gamma', 'FileSize', 'Average_Throughput'
])

# Convert numerical columns to appropriate data types
df['Alpha'] = df['Alpha'].astype(int)
df['Beta'] = df['Beta'].astype(int)
df['Gamma'] = df['Gamma'].astype(int)
df['FileSize'] = df['FileSize'].astype(int)
df['Average_Throughput'] = df['Average_Throughput'].astype(float)

# Save the DataFrame to a CSV file for initial output check
df.to_csv('parsed_results.csv', index=False)

# Create directory for visualization files
visualization_dir = 'visualization'
if not os.path.exists(visualization_dir):
    os.makedirs(visualization_dir)

# Function to plot throughput for each file size
def plot_throughput(df, file_size):
    filtered_df = df[df['FileSize'] == file_size]
    combinations = filtered_df.apply(lambda x: f"Alpha {x['Alpha']}, Beta {x['Beta']}, Gamma {x['Gamma']}", axis=1)
    throughput = filtered_df['Average_Throughput']
    
    plt.figure(figsize=(12, 8))
    bars = plt.bar(combinations, throughput, color='skyblue')
    plt.xlabel('Alpha, Beta, and Gamma Combinations')
    plt.ylabel('Average Throughput (Mbps)')
    plt.title(f'Average Throughput for File Size: {file_size}MB')
    plt.xticks(rotation=90)
    
    # Set y-axis limits to better display the differences
    min_throughput = throughput.min()
    max_throughput = throughput.max()
    plt.ylim(min_throughput - 0.01, max_throughput + 0.01)
    
    # Add text on top of the bars
    for bar in bars:
        yval = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2, yval + 0.001, round(yval, 4), ha='center', va='bottom')

    plt.tight_layout()
    plt.savefig(os.path.join(visualization_dir, f'throughput_{file_size}MB.png'))
    plt.close()

# Function to plot violin plots for each file size
def plot_violin(df, file_size):
    filtered_df = df[df['FileSize'] == file_size]
    
    plt.figure(figsize=(14, 8))
    sns.violinplot(x='Alpha', y='Average_Throughput', hue='Beta', data=filtered_df, palette='muted')
    plt.xlabel('Alpha')
    plt.ylabel('Average Throughput (Mbps)')
    plt.title(f'Violin Plot of Throughput for File Size: {file_size}MB')
    plt.tight_layout()
    plt.savefig(os.path.join(visualization_dir, f'violin_{file_size}MB.png'))
    plt.close()

# Function to plot pairplot
def plot_pairplot(df):
    sns.pairplot(df, hue='Beta', palette='muted')
    plt.tight_layout()
    plt.savefig(os.path.join(visualization_dir, 'pairplot.png'))
    plt.close()

# Function to plot bar chart for max and min results
def plot_max_min_results(max_min_results):
    sizes = []
    throughputs = []
    labels = []
    for size, results in max_min_results.items():
        sizes.append(size)
        throughputs.append(results['best']['throughput'])
        labels.append(f"Best (Size {size}MB)\nAlpha: {results['best']['alpha']}, Beta: {results['best']['beta']}, Gamma: {results['best']['gamma']}")
        sizes.append(size)
        throughputs.append(results['worst']['throughput'])
        labels.append(f"Worst (Size {size}MB)\nAlpha: {results['worst']['alpha']}, Beta: {results['worst']['beta']}, Gamma: {results['worst']['gamma']}")

    plt.figure(figsize=(14, 8))
    bars = plt.bar(labels, throughputs, color=['green', 'red'] * len(max_min_results))
    plt.xlabel('Alpha, Beta, and Gamma Combinations')
    plt.ylabel('Throughput (Mbps)')
    plt.title('Best and Worst Throughput for Each File Size')
    plt.xticks(rotation=90)
    
    for bar in bars:
        yval = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2, yval + 0.001, round(yval, 4), ha='center', va='bottom')
    
    plt.tight_layout()
    plt.savefig(os.path.join(visualization_dir, 'max_min_results.png'))
    plt.close()

# Plot throughput and violin plots for each file size
file_sizes = df['FileSize'].unique()
for size in file_sizes:
    plot_throughput(df, size)
    plot_violin(df, size)

# Plot pairplot for overall data
plot_pairplot(df)

# Save best and worst results for each file size
max_min_results = {}
with open(os.path.join(visualization_dir, 'max_min_results.txt'), 'w') as file:
    for size in file_sizes:
        filtered_df = df[df['FileSize'] == size]
        max_row = filtered_df.loc[filtered_df['Average_Throughput'].idxmax()]
        min_row = filtered_df.loc[filtered_df['Average_Throughput'].idxmin()]
        file.write(f"File Size: {size}MB\n")
        file.write(f"Best - Alpha: {max_row['Alpha']}, Beta: {max_row['Beta']}, Gamma: {max_row['Gamma']}, Throughput: {max_row['Average_Throughput']} Mbps\n")
        file.write(f"Worst - Alpha: {min_row['Alpha']}, Beta: {min_row['Beta']}, Gamma: {min_row['Gamma']}, Throughput: {min_row['Average_Throughput']} Mbps\n\n")
        max_min_results[size] = {
            'best': {
                'alpha': max_row['Alpha'],
                'beta': max_row['Beta'],
                'gamma': max_row['Gamma'],
                'throughput': max_row['Average_Throughput']
            },
            'worst': {
                'alpha': min_row['Alpha'],
                'beta': min_row['Beta'],
                'gamma': min_row['Gamma'],
                'throughput': min_row['Average_Throughput']
            }
        }

# Plot bar chart for max and min results
plot_max_min_results(max_min_results)

print("Data has been parsed and saved to parsed_results.csv. Visualization files are saved in the 'visualization' folder.")
