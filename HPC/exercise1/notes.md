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
mpirun ./c/mpi/collective/blocking/osu_bcast -x 1000 -i 25000
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
I tried to reduce the latency by binding on the same numa region also choosing the closest one to the infiniband card (the 96-111 cores). The latency between 2 processors is lower but as soon as the message size becomes more consistent it obviously does not make much of a difference. Testing this with a larger amount of processors increases the latency, even with small message sizes. I guess that during a broadcast operation other factors influence the latency, especially with a large number of processors and distributing the processors in a more even manner among the nodes reduces overall latency. The algorithm used by the broadcast operation can influence this as well.

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
The algorithm used by the osu_bcast can lead to different performances wrt the map-by, therefore an optimal solution for all algorithms to test might not exist.

To show this I experiment as follows:

I experiment the usage of (2) chain algorithm for broadcast vs 2 different map-by policies. There's a huge difference in performance using map-by socket vs core.
This makes sense as using map-by socket and a chain algorithm subsequent processors that must communicate are always far away from each other

Chain tree reference
> Chain tree algorithm. Each internal node in the topology has one child (see Fig. 3b). The message is split into segments and transmission of segments continues in a pipeline until the last node gets the broadcast message. ith process receives the message from the (i − 1)th process, and sends it to (i + 1)th process.

```bash
# Map by socket using chain algorithm
$ mpirun --mca coll_tuned_use_dynamic_rules true --mca coll_tuned_bcast_algorithm 2 -H epyc002:128,epyc003:128 --map-by socket ./c/mpi/collective/blocking/osu_bcast -x 3000 -i 100000 -m 8192

# OSU MPI Broadcast Latency Test v7.3
# Datatype: MPI_CHAR.
# Size       Avg Latency(us)
1                      32.10
2                      27.03
4                      18.58
8                      16.20
16                     16.46
32                     20.17
64                     19.58
128                    24.74
256                    26.94
512                    33.17
1024                   41.64
2048                   64.22
4096                  104.90
8192                  190.94

# Map by core using chain algorithm
$ mpirun --mca coll_tuned_use_dynamic_rules true --mca coll_tuned_bcast_algorithm 2 -H epyc002:128,epyc003:128 --map-by core ./c/mpi/collective/blocking/osu_bcast -x 3000 -i 100000 -m 8192

# OSU MPI Broadcast Latency Test v7.3
# Datatype: MPI_CHAR.
# Size       Avg Latency(us)
1                       7.77
2                       7.56
4                       7.78
8                       7.68
16                      7.49
32                      7.92
64                      7.93
128                     9.56
256                     9.45
512                    10.56
1024                   11.06
2048                   14.49
4096                   18.56
8192                   28.08
```

Notice how the same experiment using a more complex algorithm such as (5) binary tree has a different behaviour, with better performances using map-by core but with a much smaller effect (~2x vs ~4x difference).
```bash
# Map by socket using binary tree algorithm
$ mpirun --mca coll_tuned_use_dynamic_rules true --mca coll_tuned_bcast_algorithm 5 -H epyc002:128,epyc003:128 --map-by socket ./c/mpi/collective/blocking/osu_bcast -x 3000 -i 100000 -m 8192

# OSU MPI Broadcast Latency Test v7.3
# Datatype: MPI_CHAR.
# Size       Avg Latency(us)
1                       6.70
2                       6.29
4                       6.47
8                       4.82
16                      6.41
32                      4.31
64                      7.32
128                     8.29
256                     6.68
512                     8.05
1024                    8.22
2048                   17.95
4096                   20.29
8192                   29.98

# Map by core using binary tree algorithm
$ mpirun --mca coll_tuned_use_dynamic_rules true --mca coll_tuned_bcast_algorithm 5 -H epyc002:128,epyc003:128 --map-by core ./c/mpi/collective/blocking/osu_bcast -x 3000 -i 100000 -m 8192

# OSU MPI Broadcast Latency Test v7.3
# Datatype: MPI_CHAR.
# Size       Avg Latency(us)
1                       3.67
2                       3.79
4                       3.77
8                       3.77
16                      3.92
32                      4.17
64                      4.21
128                     6.08
256                     6.60
512                     6.98
1024                    7.20
2048                   10.07
4096                   18.89
8192                   35.40
```

I picked these 3 algorithms for testing broadcast:  
and I compare them with default implementation (which *tries* to use the most convenient one)
- (2) chain
- (5) binary tree
- (3) pipeline

I pick the `scatter` as the second operation for the exercise.
For scatter we do not have much choice in algorithms and we compare the default with:
- (1) basic linear
- (2) binomial
- (3) non-blocking linear