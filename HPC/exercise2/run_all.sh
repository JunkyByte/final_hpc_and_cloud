#!/bin/bash
#SBATCH --partition=EPYC
#SBATCH --job-name=hpcex2
#SBATCH --ntasks-per-node=128
#SBATCH --cpus-per-task=1
#SBATCH --nodes=2
#SBATCH --mem=450gb
#SBATCH --time=2:00:00
#SBATCH --exclusive
#SBATCH -A dssc
#SBATCH --output=./output.log  # Redirect stdout to a file

rm -rf ./results/*.csv

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <experiment>"
    exit 1
fi

experiment="$1"
script_names=("mpi_gather.c" "naive_gather.c" "naive_gather_improved.c" "gather_ring.c" "gather_binary_tree.c")

# Loop through the script names
for script in "${script_names[@]}"
do
    echo "Running script: $script"
    sbatch ./run.sh "$script" "$experiment"
done