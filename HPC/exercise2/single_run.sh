#!/bin/bash

if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <script> <num_processes> <data_size> <out_csv>"
    exit 1
fi

script="$1"
num_processes="$2"
data_size="$3"
csv_file="$4"

# Get the node names using scontrol show hostnames
node_names=$(scontrol show hostnames)

# Use tr to replace newline characters with spaces and populate the array
nodes=($(echo "$node_names" | tr '\n' ' '))

# Calculate the number of processes per node
processes_per_node=$((num_processes / ${#nodes[@]}))

# Build the host specification for mpirun
host_spec=""
for node in "${nodes[@]}"; do
    host_spec+=",$node:$processes_per_node"
done

# Remove the leading comma
host_spec="${host_spec:1}"

# Run the executable using mpirun
timing=$(mpirun --map-by core -H "$host_spec" "./build/out_${script%.c}")

# Print the timing information with alignment
printf "Processes: %-3d | Data Size: %-5d | Timing: %s\n" "$num_processes" "$data_size" "$timing"

# Append the timing information to the CSV file
echo "$num_processes,$data_size,$timing" >> "$csv_file"