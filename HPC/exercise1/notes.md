# Notes

Started by experimenting with compilation. Idea is to be able to run on different number of nodes and processes the required OSU benchmarks.

I compiled using interactive console using
```bash
srun -n128 -N1 -p EPYC --time=0:15:00 --pty bash
module load openMPI/4.1.5/gnu
# Then from build folder I compiled with
../configure CC=mpicc --prefix=$(pwd)
```

Now I try running with salloc, notes from gpt

The `salloc` command is commonly used in high-performance computing (HPC) environments to request resources and allocate them for running parallel jobs. Here's a breakdown of the command you provided:

```bash
salloc -n128 -c1 -N1 -p EPYC --time=0:15:00 -A dssc
```

- `salloc`: This command is used to request resources and obtain an allocation for running parallel jobs.

- `-n128`: This flag specifies the number of tasks (or processes) you want to allocate. In this case, you are requesting 128 tasks.

- `-c1`: This flag specifies the number of CPU cores per task. Here, you are requesting 1 core per task.

- `-N1`: This flag specifies the number of nodes you want to allocate. You are requesting 1 node.

- `-p EPYC`: This flag specifies the partition or queue you want to submit your job to. In this case, you are requesting to run your job on nodes that belong to the "EPYC" partition. Partitions are often used to categorize nodes based on their hardware characteristics.

- `--time=0:15:00`: This flag specifies the maximum time your job is allowed to run. Here, you are requesting a time limit of 15 minutes.

```bash
salloc -n128 -c1 -N1 -p EPYC --time=0:15:00 -A dssc
# From build folder
cd final/HPC/exercise1/osu-micro-benchmarks-7.3/build/
module load openMPI/4.1.5/gnu
mpirun ./c/mpi/collective/blocking/osu_bcast
```

The output is

```bash
# OSU MPI Broadcast Latency Test v7.3
# Datatype: MPI_CHAR.
# Size       Avg Latency(us)
1                       2.42
2                       3.41
4                       3.44
8                       3.43
16                      3.51
32                      4.06
64                      4.24
128                     5.67
256                     5.66
512                     6.09
1024                    6.70
2048                   10.54
4096                   19.47
8192                   35.75
16384                 116.96
32768                 177.16
65536                  95.19
131072                204.66
262144                477.93
524288               1074.34
1048576              1858.65
```
I notice that  the times are a bit inconsistent (1 node 128 cores). I'm requesting all cores for this particular task from my epyc node but I did not require exclusivity (I think). It might also be related to low amount of repetitions for the test.

```bash
# Running with higher warmup and higher number of repetitions is more consistent but requires more time.
# defaults were -x 200 (warmup) and -i 10000 (iterations)
mpirun ./c/mpi/collective/blocking/osu_bcast -x 1000 -i 100000
# Using exclusive seems to give a bit better results.
# just add --exclusive to salloc
```


Help of osu_bcast executable
```bash
[adonninelli@login01 build]$ ./c/mpi/collective/blocking/osu_bcast --help
Usage: osu_bcast [options]
Options:
  -h, --help                print this help
  -v, --version             print version info
  -f, --full                print full format listing (MIN/MAX latency and ITERATIONS
                            displayed in addition to AVERAGE latency)
  -m, --message-size        [MIN:]MAX - set the minimum and/or the maximum message size to MIN and/or MAX
                            bytes respectively. Examples:
                            -m 128      // min = default, max = 128
                            -m 2:128    // min = 2, max = 128
                            -m 2:       // min = 2, max = default
  -i, --iterations          ITER - number of iterations for timing (default 10000)
  -x, --warmup              ITER - set number of warmup
                            iterations to skip before timing (default 200)
  -M, --mem-limit           SIZE - set per process maximum memory consumption to SIZE bytes
                            (default 536870912)
  -a, --array-size          SIZE - set the size of arrays to be allocated on device (GPU)
                            for dummy compute on device (GPU) (default 32). OMB must be configured with CUDA support.
  -c, --validation          Enable or disable validation. Disabled by default.
  -u, --validation-warmup   ITR Set number of warmup iterations to skip before timing when validationis enabled (default 5)
  -G, --graph               [tty,png,pdf] - graph output of per iteration values.
  -D, --ddt                 [TYPE]:[ARGS] - Enable DDT support
                            -D cont                          //Contiguous
                            -D vect:[stride]:[block_length]  //Vector
                            -D indx:[ddt file path]          //Index
  -P, --papi                [EVENTS]:[PATH] - Enable PAPI support. OMB must be configured with CUDA support.
                            [EVENTS]       //Comma seperated list of PAPI events
                            [PATH]         //PAPI output file path
  -T, --type                [all,mpi_char,mpi_int,mpi_float] - Set MPI_TYPE . Default:MPI_CHAR.
  -I, --session             Enable session based MPI initialization.
  -z, --tail-lat            Print tail latencies. Outputs P99, P90, P50 percentiles
```


The way I want to run simulation is with same amount of cores between the two nodes.
```bash
# For testing I alloc with
salloc -n64 --ntasks-per-node=32 -c1 -N2 -p EPYC --time=0:15:00 -A dssc
# Here an example with 64 cores in total partitioned as 32 and 32 between the nodes
# To get node names required for next command we can use
scontrol show hostnames
# epyc005
# epyc006
mpirun -np 64 -H node1:32,node2:32 ./your_mpi_program
```

(Took inspiration by [orfeo intel benchmark](https://orfeo-doc.areasciencepark.it/examples/MPI-communication/#more-on-node-mapping))
I tried to reduce the latency by binding on the same numa region (the 96-111 cores) also choosing the closest one to the infiniband card. I did not obtain better results (with large messages I got worse results). Still running the latency test they have a lower latency.
I guess that during a broadcast operation other factors influence the latency, especially with a large number of processors.

Notice that with 2 processors the latency (reported for small message sizes) is lower when binding to these cores

```bash
# myrankfile content
# rank 0=epyc001 slot=96-111
# rank 1=epyc002 slot=96-111
$ mpirun --report-bindings --rankfile myrankfile ./c/mpi/collective/blocking/osu_bcast -m 1024 -i 100000 -x 3000

# OSU MPI Broadcast Latency Test v7.3
# Datatype: MPI_CHAR.
# Size       Avg Latency(us)
1                       1.55
2                       1.55
4                       1.56
8                       1.55
16                      1.56
32                      1.62
64                      2.00
128                     1.70
256                     2.09
512                     2.14
1024                    2.64

$ mpirun --report-bindings -H epyc002:1,epyc001:1 ./c/mpi/collective/blocking/osu_bcast -m 1024 -i 100000 -x 3000

# OSU MPI Broadcast Latency Test v7.3
# Datatype: MPI_CHAR.
# Size       Avg Latency(us)
1                       1.77
2                       1.79
4                       1.78
8                       1.77
16                      1.78
32                      1.87
64                      2.27
128                     1.85
256                     2.42
512                     2.41
1024                    2.95
```

Therefore I won't write a custom rankfile for the benchmark but only use the `map-by` checking for lowest latency achievable.