import re
import pandas as pd
import matplotlib.pyplot as plt

# Read the content of the results.txt file
with open('results.txt', 'r') as file:
    content = file.read()

# Define the pattern to match the entries
pattern = re.compile(
    r'Alpha: (\d+) Beta: (\d+) FileSize: (\d+MB)\n'
    r'Flow ID: (\d+) Src Addr (\d+\.\d+\.\d+\.\d+) Dst Addr (\d+\.\d+\.\d+\.\d+)\n'
    r'Tx Packets = (\d+)\nRx Packets = (\d+)\nDuration: ([\d\.]+)\nThroughput: ([\d\.]+) Mbps'
)

# Extract the data using the pattern
matches = pattern.findall(content)

# Create a DataFrame from the extracted data
df = pd.DataFrame(matches, columns=[
    'Alpha', 'Beta', 'FileSize', 'Flow_ID', 'Src_Addr', 'Dst_Addr', 
    'Tx_Packets', 'Rx_Packets', 'Duration', 'Throughput'
])

# Convert numerical columns to appropriate data types
df['Alpha'] = df['Alpha'].astype(int)
df['Beta'] = df['Beta'].astype(int)
df['FileSize'] = df['FileSize'].apply(lambda x: int(x.replace('MB', '')))
df['Flow_ID'] = df['Flow_ID'].astype(int)
df['Tx_Packets'] = df['Tx_Packets'].astype(int)
df['Rx_Packets'] = df['Rx_Packets'].astype(int)
df['Duration'] = df['Duration'].astype(float)
df['Throughput'] = df['Throughput'].astype(float)

# Save the DataFrame to a CSV file for initial output check
df.to_csv('parsed_results.csv', index=False)

# Function to plot throughput for each file size
def plot_throughput(df, file_size):
    filtered_df = df[df['FileSize'] == file_size]
    combinations = filtered_df.apply(lambda x: f"Alpha {x['Alpha']}, Beta {x['Beta']}", axis=1)
    throughput = filtered_df['Throughput']
    
    plt.figure(figsize=(10, 6))
    plt.bar(combinations, throughput, color='skyblue')
    plt.xlabel('Alpha and Beta Combinations')
    plt.ylabel('Throughput (Mbps)')
    plt.title(f'Throughput for File Size: {file_size}MB')
    plt.xticks(rotation=90)
    plt.tight_layout()
    plt.show()

# Plot throughput for each file size
file_sizes = df['FileSize'].unique()
for size in file_sizes:
    plot_throughput(df, size)

print("Data has been parsed and saved to parsed_results.csv")
