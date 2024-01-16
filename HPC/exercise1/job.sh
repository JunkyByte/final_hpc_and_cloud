#!/bin/bash

#SBATCH --partition=EPYC
#SBATCH --job-name=tests
#  #SBATCH --ntasks=128  # 128 TODO  commented?
#SBATCH --ntasks-per-node=16  # 64 TODO
#SBATCH --cpus-per-task=1
#SBATCH --nodes=2
#SBATCH --time=0:15:00
#SBATCH -A dssc
#SBATCH --output=./output.log  # Redirect stdout to a file

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
python $python_script