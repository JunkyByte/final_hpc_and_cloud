import os
import subprocess
import pandas as pd
from subprocess import PIPE
from functools import partial

BUILD_DIRECTORY = "./osu-micro-benchmarks-7.3/build/"

BROADCAST_PATH = os.path.join(BUILD_DIRECTORY, "c/mpi/collective/blocking/osu_bcast")

# Broadcast params
WARMUP_ITERATIONS = 10  # 1000
ITERATIONS = 50  # 10000

BROADCAST_COMMAND = f'{BROADCAST_PATH} -x {WARMUP_ITERATIONS} -i {ITERATIONS} -f'

# Utility runner
SRUN = partial(subprocess.run, shell=True, stdout=PIPE, stderr=PIPE)


def mpirun_caller(code: str, nodes: list, np: int):
    assert np % len(nodes) == 0, 'The number of process does not divide the number of nodes'
    process_per_host = np // len(nodes)
    nodes_string = ''.join(f'{name}:{process_per_host},' for name in nodes)[:-1]
    full_command = f"mpirun -H {nodes_string} {code}"
    print(f'$ {full_command}')
    return SRUN(full_command)


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
    print('>>> Obtaining the nodes name')

    # Start by obtaining resources list
    ret = SRUN('scontrol show hostnames')
    assert ret.returncode == 0, 'Could not get hostnames, are resources allocated?'

    nodes = ret.stdout.decode().split('\n')[:-1]
    print(f'>>> Nodes found: {nodes}')
    assert len(nodes) == 2, 'Number of nodes != 2, is it okay?'


    # LOGIC FOR NP TODO
    # TODO: NPS = [2, 4, 8, 16, 32, 64, 128, 256]

    # These are total amount of process and will be divided between the nodes equally
    NPS = [2, 4, 8, 16]

    parsed_outputs = []
    for NP in NPS:
        print(f'>>> Running BROADCAST with {NP} total processes')
        ret = mpirun_caller(BROADCAST_COMMAND, nodes, NP)
        print(f'>>> Command succeeded: {ret.returncode == 0}')
        assert ret.returncode == 0

        output = ret.stdout.decode()
        df = parse_output(output)
        df['NP_total'] = NP
        parsed_outputs.append(df)

    # Concatenate all DataFrames into a single large DataFrame
    result_df = pd.concat(parsed_outputs, ignore_index=True)
    result_df.to_csv('./results.csv', index=False)
    
    print('>>>>>> Correctly terminated execution :)')