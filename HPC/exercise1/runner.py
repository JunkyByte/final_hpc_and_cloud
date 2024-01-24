import os
import subprocess
import pandas as pd
from subprocess import PIPE
from functools import partial
import argparse

BUILD_DIRECTORY = "./osu-micro-benchmarks-7.3/build/"

BROADCAST_PATH = os.path.join(BUILD_DIRECTORY, "c/mpi/collective/blocking/osu_bcast")
SCATTER_PATH = os.path.join(BUILD_DIRECTORY, "c/mpi/collective/blocking/osu_scatter")

# Common number of iterations
WARMUP_ITERATIONS = 1000
ITERATIONS = 40000

# Broadcast params
WARMUP_ITERATIONS_BCAST = WARMUP_ITERATIONS
ITERATIONS_BCAST = ITERATIONS
BROADCAST_COMMAND = f'{BROADCAST_PATH} -x {WARMUP_ITERATIONS_BCAST} -i {ITERATIONS_BCAST} -f'
BROADCAST_ALGO = '--mca coll_tuned_use_dynamic_rules true --mca coll_tuned_bcast_algorithm {0}'  # Note these must be appended to mpirun
BROADCAST_ALGO_CODES = [0, 2, 3, 5]
BROADCAST_ALGO_NAMES = {0: 'default', 2: 'chain', 5: 'binary tree', 3: 'pipeline'}

# Scatter params
WARMUP_ITERATIONS_SCATTER = WARMUP_ITERATIONS
ITERATIONS_SCATTER = ITERATIONS
SCATTER_COMMAND = f'{SCATTER_PATH} -x {WARMUP_ITERATIONS_SCATTER} -i {ITERATIONS_SCATTER} -f'
SCATTER_ALGO = '--mca coll_tuned_use_dynamic_rules true --mca coll_tuned_scatter_algorithm {0}'  # Note these must be appended to mpirun
SCATTER_ALGO_CODES = [0, 1, 2, 3]
SCATTER_ALGO_NAMES = {0: 'default', 1: 'basic linear', 2: 'binomial', 3: 'non-blocking linear'}

# Utility runner
SRUN = partial(subprocess.run, shell=True, stdout=PIPE, stderr=PIPE)


def mpirun_caller(code: str, algo_code: str, nodes: list, np: int):
    assert np % len(nodes) == 0, 'The number of process does not divide the number of nodes'
    process_per_host = np // len(nodes)
    nodes_string = ''.join(f'{name}:{process_per_host},' for name in nodes)[:-1]
    full_command = f"mpirun --map-by core {algo_code} -H {nodes_string} {code}"
    print(f'$ {full_command}', flush=True)
    return SRUN(full_command)


def run_experiment(COMMAND, ALGOS_COMMAND, ALGOS, ALGOS_NAME, NPS, OP_NAME=None):
    parsed_outputs = []
    k = 0
    tot = len(ALGOS) * len(NPS)
    for ALGO in ALGOS:
        ALGO_COMMAND = ALGOS_COMMAND.format(ALGO)
        ALGO_NAME = ALGOS_NAME[ALGO]
        for NP in NPS:
            k += 1
            print(f'>>> {k}/{tot} Running {OP_NAME} with {NP} total processes', flush=True)
            ret = mpirun_caller(COMMAND, ALGO_COMMAND, nodes, NP)
            print(f'>>> Command succeeded: {ret.returncode == 0}', flush=True)
            assert ret.returncode == 0

            output = ret.stdout.decode()
            df = parse_output(output)
            df['NP_total'] = NP
            df['ALGO'] = ALGO
            df['ALGO_NAME'] = ALGO_NAME
            parsed_outputs.append(df)
    
    # Concatenate all DataFrames into a single large DataFrame
    result_df = pd.concat(parsed_outputs, ignore_index=True)
    return result_df


# Function to parse the output and create a DataFrame
def parse_output(output):
    data = []
    lines = output.strip().split('\n')[3:]
    for line in lines:
        values = line.split()
        data.append([int(values[0])] + [float(v) for v in values[1:-1]] + [int(values[-1])])
    df = pd.DataFrame(data, columns=['Size', 'Avg Latency(us)', 'Min Latency(us)', 'Max Latency(us)', 'Iterations'])
    return df


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Perform scatter or broadcast operation.")
    parser.add_argument('operation', choices=['scatter', 'broadcast'], help="Choose either 'scatter' or 'broadcast'.")
    args = parser.parse_args()
    operation = args.operation.lower()

    print(f'>>> Will perform latency for operation: {operation}')
    print('>>> Obtaining the nodes name', flush=True)

    # Start by obtaining resources list
    ret = SRUN('scontrol show hostnames')
    assert ret.returncode == 0, 'Could not get hostnames, are resources allocated?'

    nodes = ret.stdout.decode().split('\n')[:-1]
    print(f'>>> Nodes found: {nodes}', flush=True)
    assert len(nodes) == 2, 'Number of nodes != 2, is it okay?'

    # These are total amount of process and will be divided between the nodes equally
    NPS = [2, 4, 8, 16, 32, 64, 128, 256]

    if operation == 'scatter':
        out_df_scatter = run_experiment(SCATTER_COMMAND, SCATTER_ALGO, SCATTER_ALGO_CODES, SCATTER_ALGO_NAMES, NPS, 'SCATTER')
        out_df_scatter.to_csv('./results_scatter.csv', index=False)
    elif operation == 'broadcast':
        out_df_bcast = run_experiment(BROADCAST_COMMAND, BROADCAST_ALGO, BROADCAST_ALGO_CODES, BROADCAST_ALGO_NAMES, NPS, 'BCAST')
        out_df_bcast.to_csv('./results_bcast.csv', index=False)

    print('>>>>>> Correctly terminated execution :)', flush=True)