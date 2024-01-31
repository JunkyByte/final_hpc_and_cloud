#!/bin/bash

script_names=("mpi_gather.c" "naive_gather.c" "naive_gather_improved.c" "gather_ring.c" "gather_ring_all_parallel.c" "gather_binary_tree.c")
num_processes=32
num_repetitions=1

# Get the SEND_COUNT value
SEND_COUNT="$1"

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
    done
done
