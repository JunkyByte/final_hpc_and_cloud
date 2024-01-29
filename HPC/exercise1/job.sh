#!/bin/bash

#SBATCH --partition=EPYC
#SBATCH --job-name=hpcex
#SBATCH --ntasks-per-node=128
#SBATCH --cpus-per-task=1
#SBATCH --nodes=2
#SBATCH --time=2:00:00
#SBATCH --exclusive
#SBATCH -A dssc
#SBATCH --output=./output.log  # Redirect stdout to a file

date

cd /u/dssc/adonninelli/final/HPC/exercise1

# Load necessary modules
echo ">>> Loading needed modules!"
module load openMPI/4.1.5/gnu
module load conda

# Install pandas
echo ">>> Installing pandas dependency"
pip install pandas --user

# Set the path to the Python script
python_script="./runner.py"

# Run the Python script with sbatch
echo ">>> Running python script"
python $python_script $1 $2

date