#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <num_repetitions>"
    exit 1
fi

num_repetitions="$1"
script_names=("mpi_gather.c" "naive_gather.c" "naive_gather_improved.c" "gather_ring.c" "gather_ring_all_parallel.c" "gather_binary_tree.c")
max_num_processes=36
max_data_size=32768

# Loop through the script names
for script in "${script_names[@]}"
do
    echo "Running script: $script"
    sh ./run.sh "$script" "$num_repetitions"
done