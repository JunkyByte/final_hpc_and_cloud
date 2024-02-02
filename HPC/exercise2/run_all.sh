#!/bin/bash

#SBATCH --partition=EPYC
#SBATCH --job-name=hpcex_2
#SBATCH --ntasks-per-node=128
#SBATCH --cpus-per-task=1
#SBATCH --nodes=2
#SBATCH --mem-per-cpu=3gb
#SBATCH --time=2:00:00
#SBATCH --exclusive
#SBATCH -A dssc
#SBATCH --output=./output.log  # Redirect stdout to a file

date

cd /u/dssc/adonninelli/final/HPC/exercise2

# Load necessary modules
echo ">>> Loading needed modules!"
module load openMPI/4.1.5/gnu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <num_repetitions>"
    exit 1
fi

num_repetitions="$1"
script_names=("mpi_gather.c" "naive_gather.c" "naive_gather_improved.c" "gather_ring.c" "gather_ring_all_parallel.c" "gather_binary_tree.c")

# Loop through the script names
for script in "${script_names[@]}"
do
    echo "Running script: $script"
    sh ./run.sh "$script" "$num_repetitions"
done

date