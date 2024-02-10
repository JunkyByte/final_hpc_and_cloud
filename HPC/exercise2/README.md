# Exercise 2A - HPC Final Assignment
# Gather operation using p2p communication.

Please refer to `exercise2.md` for the text of the exercise.
`report.pdf` contains the final report.

In order to obtain the reported timings `run_all.sh` can be used. The experiments are divided into 4 for the different fixed values in weak and strong scaling.

```bash
sbatch run_all.sh [experiment]
```

Where experiment in `[0, 1, 2, 3]`. Check `run.sh` for further details.
The results are saved in `results/`