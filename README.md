# TCP Vegas Simulation

This repository contains an ns-3 simulation script to evaluate the performance of TCP Vegas under various alpha and beta parameters. The script tests combinations of alpha and beta values ranging from 1 to 10, along with different file sizes (1MB, 100MB, 1GB, 100GB, 1TB, 100TB). Performance metrics such as average throughput are logged for each combination to analyze the efficiency of TCP Vegas.

# Simulation Details

## Parameters
- Alpha Values: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
- Beta Values: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 (greater or equal to alpha)
- Gamma Values: 1, 2, 3
- File Sizes: 1MB, 100MB, 1GB, 100GB, 1TB, 100TB
- Packet Size: 1040 bytes
- Number of Runs: 10,000
- Packet Loss Rate: 1%


# Setup

To run the simulation:

1. Place the vegas_simulation.cc script in the scratch directory of ns-3.
2. Build the script using waf.
3. Execute the simulation.

# Example setup using docker:

1. Pull the Docker image (https://github.com/hygorjardim/ns3-docker):
```
docker pull hygorjardim/ns3:3.0
```

2. Start a Docker container:
```
docker run -ti --privileged --network=host -p 5060:5060 --name ns3 hygorjardim/ns3:3.0
```

3. Copy the simulation script into the container:
```
docker cp vegas_simulation.cc ns3:/usr/ns-allinone-3.30/ns-3.30/scratch/
```

4. Enter the container:
```
docker exec -ti ns3 /bin/bash
```

5. Build the simulation script inside the container:
```
cd /usr/ns-allinone-3.30/ns-3.30/
./waf build
```

6. Run the simulation
```
./waf --run scratch/vegas_simulation -v
```

## Output format:

The simulation logs the average throughput for each combination of alpha, beta, and file size into a results.txt file. The format of the output is as follows:
```
Alpha: <alpha> Beta: <beta> FileSize: <fileSize>MB Average Throughput: <throughput> Mbps
```
### Sample of real output:
```
Alpha: 1 Beta: 1 FileSize: 1MB Average Throughput: 0.200992 Mbps
Alpha: 1 Beta: 1 FileSize: 100MB Average Throughput: 0.200373 Mbps
Alpha: 1 Beta: 1 FileSize: 1000MB Average Throughput: 0.197651 Mbps
...
```

# Example visualization

A Python script "analyze_vegas_results.py" is provided to parse the results and visualize the average throughput for different file sizes.
