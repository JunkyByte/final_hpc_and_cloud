#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <script> <num_repetitions>"
    exit 1
fi

num_repetitions="$2"
script="$1"

max_num_processes=256
# 4194304 is ~16mb per process - 16777216 is ~67mb per process - 33554432 is ~135mb per process
max_data_size=4194304

# Using smaller values leads to latency bounded tests..
# I do 250000 which is 1mb of data per task / 2500000 which is 10mb of data per task / 7500000 which is 30mb of data per task
fixed_data_size=250  # 7500000 # 2500000 # 250000

# I run with 32 / 64 / 256
fixed_num_processes=256 # 64  # 32

csv_file="./results/${script%.c}_results_weakscaling_$fixed_data_size.csv"

# Weak Scaling - Varying the number of participating tasks and fixing the amount of data per task
# I start from 4 as with 2 I get inconsistent results. Data size is small..
mpirun -np 1 mpicc -march=native -lm -O3 -D SEND_COUNT="$fixed_data_size" "$script" -o "./build/out_${script%.c}"
for ((num_processes=4; num_processes<=$max_num_processes; num_processes*=2))
do
    # Call Script 2 with required parameters
    printf "> Weak Scaling - %s - Num Processes: %d - Data Size: %d - Fixed Data Size: %d\n" "$script" "$num_processes" "$fixed_data_size" "$fixed_data_size"
    sh ./single_run.sh "$script" "$num_processes" "$fixed_data_size" "$num_repetitions" "$csv_file"
done


# csv_file="./results/${script%.c}_results_strongscaling_$fixed_num_processes.csv"
# 
# # Strong Scaling - Varying the amount of data per task while keeping the amount of tasks
# # With very small data_size its latency bounded and results are not interesting
# # so I start with larger than 1
# for ((data_size=256; data_size<=$max_data_size; data_size*=2))
# do
#     # Compile it
#     mpirun -np 1 mpicc -march=native -lm -O3 -D SEND_COUNT="$data_size" "$script" -o "./build/out_${script%.c}"
# 
#     # Call Script 2 with required parameters
#     printf "> Strong Scaling - %s - Num Processes: %d - Data Size: %d - Fixed Num Processes: %d\n" "$script" "$fixed_num_processes" "$data_size" "$fixed_num_processes"
#     sh ./single_run.sh "$script" "$fixed_num_processes" "$data_size" "$num_repetitions" "$csv_file"
# done
