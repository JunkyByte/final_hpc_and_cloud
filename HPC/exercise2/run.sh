#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <script> <num_repetitions>"
    exit 1
fi

num_repetitions="$2"
script="$1"

max_num_processes=256
max_data_size=268435456 # * 4 bytes - is a max_data_size of ~1 gb per process

# I do 250 which is 1kb of data per task / 250000 which is 1mb of data per task / 250000000 which is 1gb of data per task
fixed_data_size=250

# I run with 32 / 128 / 256
fixed_num_processes=32

csv_file="./results/${script%.c}_results_weakscaling_$fixed_data_size.csv"

# Weak Scaling - Varying the number of participating tasks and fixing the amount of data per task
for ((num_processes=2; num_processes<=$max_num_processes; num_processes*=2))
do
    # Compile it
    mpirun -np 1 mpicc -march=native -lm -O3 -D SEND_COUNT="$fixed_data_size" "$script" -o "./build/out_${script%.c}"

    # Call Script 2 with required parameters
    printf "> Weak Scaling - %s - Num Processes: %d - Data Size: %d - Fixed Data Size: %d\n" "$script" "$num_processes" "$fixed_data_size" "$fixed_data_size"
    sh ./single_run.sh "$script" "$num_processes" "$fixed_data_size" "$num_repetitions" "$csv_file"
done

csv_file="./results/${script%.c}_results_strongscaling_$fixed_num_processes.csv"

# Strong Scaling - Varying the amount of data per task while keeping the amount of tasks
for ((data_size=1; data_size<=$max_data_size; data_size*=2))
do
    # Compile it
    mpirun -np 1 mpicc -march=native -lm -O3 -D SEND_COUNT="$data_size" "$script" -o "./build/out_${script%.c}"

    # Call Script 2 with required parameters
    printf "> Strong Scaling - %s - Num Processes: %d - Data Size: %d - Fixed Num Processes: %d\n" "$script" "$fixed_num_processes" "$data_size" "$fixed_num_processes"
    sh ./single_run.sh "$script" "$fixed_num_processes" "$data_size" "$num_repetitions" "$csv_file"
done
