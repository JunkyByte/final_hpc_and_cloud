#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <script> <num_repetitions>"
    exit 1
fi

num_repetitions="$2"
script="$1"
max_num_processes=36
max_data_size=268435456  # ~1 gb of data
# max_data_size=2147483648  # ~8 gb of data

# Weak Scaling Loop
for ((num_processes=2; num_processes<=$max_num_processes; num_processes*=2))
do
    # Calculate the data size for weak scaling
    data_size=$((max_data_size / num_processes))

    # Compile it
    mpirun -np 1 mpicc -march=native -lm -O3 -D SEND_COUNT="$data_size" "$script" -o "./build/out_${script%.c}"

    # Call Script 2 with required parameters
    printf "> Weak Scaling - %s - Num Processes: %d - Data Size per Process: %d\n" "$script" "$num_processes" "$data_size"
    sh ./single_run.sh "$script" "$num_processes" "$data_size" "$num_repetitions"
done

# Strong Scaling Loop
# I use 250 which results in 1kb of data per process
# And 250000 which results in 1mb of data per process
data_size=250
# data_size=250000
for ((num_processes=2; num_processes<=$max_num_processes; num_processes*=2))
do
    # Compile it
    mpirun -np 1 mpicc -march=native -lm -O3 -D SEND_COUNT="$data_size" "$script" -o "./build/out_${script%.c}"

    # Call Script 2 with required parameters
    printf "> Strong Scaling - %s - Num Processes: %d - Data Size: %d\n" "$script" "$num_processes" "$data_size"
    sh ./single_run.sh "$script" "$num_processes" "$data_size" "$num_repetitions"
done
