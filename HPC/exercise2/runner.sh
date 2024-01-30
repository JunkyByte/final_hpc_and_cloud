#!/bin/bash

script_names=("mpi_gather.c" "naive_gather.c" "naive_gather_improved.c" "gather_ring.c" "gather_ring_all_parallel.c")
num_processes=36
num_repetitions=1

# Get the SEND_COUNT value
SEND_COUNT="$1"

# Create a CSV file for storing results with SEND_COUNT in the name
csv_file="timing_results_${SEND_COUNT}.csv"

# Print CSV header
echo "Algorithm,Time" > "$csv_file"

# Compile
for script in "${script_names[@]}"
do
    # Compile the C script with conditional flags
    if [ -n "$1" ]; then
        mpirun -np 1 mpicc -march=native -lm -O3 -D SEND_COUNT="$1" "$script" -o "./build/out_${script%.c}"
    else
        mpirun -np 1 mpicc -march=native -lm -O3 "$script" -o "./build/out_${script%.c}"
    fi
done

# Loop through the script names
for script in "${script_names[@]}"
do
    for ((i=1; i<=$num_repetitions; i++))
    do
        # Run the executable using mpirun
        timing=$(mpirun --map-by core -np "$num_processes" "./build/out_${script%.c}")

        # Print the timing information with alignment
        printf "Run %d - Timing for %-30s: %s\n" "$i" "$script" "$timing"

        # Extract the timing value and append to the CSV file
        echo "${script%.c},$timing" >> "$csv_file"
    done
done
