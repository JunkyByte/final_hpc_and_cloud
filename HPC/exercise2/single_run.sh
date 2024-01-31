#!/bin/bash

if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <script> <num_processes> <data_size> <num_repetitions>"
    exit 1
fi

script="$1"
num_processes="$2"
data_size="$3"
num_repetitions="$4"

# Create or append to the CSV file for storing results with script name
csv_file="./results/${script%.c}_results.csv"

# Loop through the number of repetitions
for ((rep=1; rep<=$num_repetitions; rep++))
do
    # Run the executable using mpirun
    timing=$(mpirun --map-by node -np "$num_processes" "./build/out_${script%.c}")

    # Print the timing information with alignment
    printf "Repetition %d - Processes: %-3d | Data Size: %-5d | Timing: %s\n" "$rep" "$num_processes" "$data_size" "$timing"

    # Append the timing information to the CSV file
    echo "$rep,$num_processes,$data_size,$timing" >> "$csv_file"
done
