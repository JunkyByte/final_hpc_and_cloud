#!/bin/bash

#SBATCH --partition=EPYC
#SBATCH --job-name=hpcex2
#SBATCH --ntasks-per-node=128
#SBATCH --cpus-per-task=1
#SBATCH --nodes=2
#sBATCH --mem=450gb
#SBATCH --time=2:00:00
#SBATCH --exclusive
#SBATCH -A dssc
#SBATCH --output=./output.log  # Redirect stdout to a file

# Capture the start time
start_time=$(date +%s)

module purge
module load openMPI/4.1.5/gnu
cd /u/dssc/adonninelli/final/HPC/exercise2

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <script> <experiment>"
    exit 1
fi

script="$1"
experiment="$2"

# 4194304 is ~16mb per process - 16777216 is ~67mb per process - 33554432 is ~135mb per process
# 250000 which is 1mb of data per task / 2500000 which is 10mb of data per task / 7500000 which is 30mb of data per task
if [ $experiment -eq 0 ]; then
    fixed_num_processes=8
    fixed_data_size=250  # 1 kb of data
    max_data_size=134217728  # ~536 mb
    repetitions_weak=10000
elif [ $experiment -eq 1 ]; then
    fixed_num_processes=16
    fixed_data_size=2500  # 10 kb of data
    max_data_size=67108864  # ~268 mb
    repetitions_weak=5000
elif [ $experiment -eq 2 ]; then
    fixed_num_processes=32
    fixed_data_size=250000
    max_data_size=33554432
    repetitions_weak=1000
elif [ $experiment -eq 3 ]; then
    fixed_num_processes=64
    fixed_data_size=2500000
    max_data_size=16777216
    repetitions_weak=250
elif [ $experiment -eq 4 ]; then
    fixed_num_processes=256
    fixed_data_size=7500000
    max_data_size=4194304
    repetitions_weak=100
else
    echo "Invalid experiment number!"
    exit 1
fi

max_num_processes=256

csv_file="./results/${script%.c}_results_weakscaling_$fixed_data_size.csv"

# Weak Scaling - Varying the number of participating tasks and fixing the amount of data per task
mpirun -np 1 mpicc -march=native -lm -O3 -D SEND_COUNT="$fixed_data_size" -D REPETITIONS="$repetitions_weak" "$script" -o "./build/out_${script%.c}"
for ((num_processes=2; num_processes<=$max_num_processes; num_processes*=2))
do
    # Call Script 2 with required parameters
    printf "> Weak Scaling - %s - Num Processes: %d - Data Size: %d - Fixed Data Size: %d\n" "$script" "$num_processes" "$fixed_data_size" "$fixed_data_size"
    sh ./single_run.sh "$script" "$num_processes" "$fixed_data_size" "$csv_file"
done

csv_file="./results/${script%.c}_results_strongscaling_$fixed_num_processes.csv"

# Strong Scaling - Varying the amount of data per task while keeping the amount of tasks
for ((data_size=1; data_size<=$max_data_size; data_size*=2))
do
    if [ $data_size -le 16384 ]; then
        repetitions=10000
    elif [ $data_size -le 262144 ]; then
        repetitions=1000
    elif [ $data_size -le 2097152 ]; then
        repetitions=100
    else
        repetitions=10
    fi

    # Compile it
    mpirun -np 1 mpicc -march=native -lm -O3 -D SEND_COUNT="$data_size" -D REPETITIONS="$repetitions" "$script" -o "./build/out_${script%.c}"

    # Call Script 2 with required parameters
    printf "> Strong Scaling - %s - Num Processes: %d - Data Size: %d - Fixed Num Processes: %d\n" "$script" "$fixed_num_processes" "$data_size" "$fixed_num_processes"
    sh ./single_run.sh "$script" "$fixed_num_processes" "$data_size" "$csv_file"
done

end_time=$(date +%s)
total_time=$((end_time - start_time))

echo ">>> Finished $script experiment $experiment - Took: $total_time seconds"