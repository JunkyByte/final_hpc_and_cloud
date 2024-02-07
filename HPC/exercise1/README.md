# Exercise 1 - HPC

In order to obtain the reported timings `job.sh` can be used.
The work is split into different chunks as the 2 hour limit on orfeo were not long enough to perform all the benchmarks.

```bash
sbatch job.sh [operation] [type]
```

Where operation can be:

- broadcast (for broadcast operation)
- scatter1 (half of the algorithms of scatter)
- scatter2 (the second half of the algorithms of scatter)

The results are saved in `results/`

And type can be:

- full
- fixedsize

*Full* will run the benchmarks with #processes:
`[2, 4, 8, 16, 32, 64, 128, 256]` and varying data sizes.
It is the test performed to obtain the heatmaps provided in the report.

*Fixed* will run the benchmark with #processes:
`[2 * (i + 1) for i in range(128)]` and fixed data size of 4.
It is the test performed to obtain all the plots provided in the report.