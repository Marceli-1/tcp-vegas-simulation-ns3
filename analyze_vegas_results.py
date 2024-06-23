import re
import pandas as pd
import matplotlib.pyplot as plt

# Read the content of the results.txt file
with open('results.txt', 'r') as file:
    content = file.read()

# Define the pattern to match the entries
pattern = re.compile(
    r'Alpha: (\d+) Beta: (\d+) FileSize: (\d+)MB Average Throughput: ([\d\.]+) Mbps'
)

# Extract the data using the pattern
matches = pattern.findall(content)

# Create a DataFrame from the extracted data
df = pd.DataFrame(matches, columns=[
    'Alpha', 'Beta', 'FileSize', 'Average_Throughput'
])

# Convert numerical columns to appropriate data types
df['Alpha'] = df['Alpha'].astype(int)
df['Beta'] = df['Beta'].astype(int)
df['FileSize'] = df['FileSize'].astype(int)
df['Average_Throughput'] = df['Average_Throughput'].astype(float)

# Save the DataFrame to a CSV file for initial output check
df.to_csv('parsed_results.csv', index=False)

# Function to plot throughput for each file size
def plot_throughput(df, file_size):
    filtered_df = df[df['FileSize'] == file_size]
    combinations = filtered_df.apply(lambda x: f"Alpha {x['Alpha']}, Beta {x['Beta']}", axis=1)
    throughput = filtered_df['Average_Throughput']
    
    plt.figure(figsize=(10, 6))
    bars = plt.bar(combinations, throughput, color='skyblue')
    plt.xlabel('Alpha and Beta Combinations')
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
    plt.show()

# Plot throughput for each file size
file_sizes = df['FileSize'].unique()
for size in file_sizes:
    plot_throughput(df, size)

print("Data has been parsed and saved to parsed_results.csv")
