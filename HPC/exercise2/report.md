# Final Project HPC - Exercise 2
# Implementing Gather  using p2p communication (Ex. 2A)
- Adriano Donninelli adonnine@sissa.it

## Introduction

I have chosen to implement a custom version of the Gather collective operation using point-to-point (p2p) communication in the context of MPI (Message Passing Interface). The gather operation involves the root process collecting data from all other processes. For semplicity I aim to share an array of integers of fixed size from all processes to the root process. Extending the implementations I provide to support tags, custom group communicators and arbitrary data types its quite trivial as p2p communication support all these features but was left out for simplicity as we pose more attention on the timings of the different solutions.

I will compare my custom gather operations with the different algorithms of the MPI_Gather implementation provided by OpenMPI. For simplicity, in some of the scripts I will assume that root process is rank 0.

## Gather operation

Before proceding let's define how the Gather collective operation works:

Suppose that there are $$N$$ processes and that each has a fixed amount of data with size $$K$$.
Performing a gather operations means that one particular process, denoted as root receives from all the other processes their chunk of data. At the end of a gather operation the root process will hold a buffer of data of size $$N * K$$.
The gathered data must be ordered as the ranks assigned to the processes.

![gather_fig](https://i.imgur.com/efTku4k.png)

## Experimental Setup

In order to obtain more consistent timings I perform multiple runs of all the algorithms and average the timings, I also perform a warmup of the communication channels passing dummy data between the processes. For each solution I only time the actual gather operation and not the variable setup required by different algorithms. To perform all the timings I write a set of bash scripts which allow to perform weak and strong scaling measurements of the different implementations. The tests are performed on ORFEO cluster using the epyc partition. A varying amount of nodes and data sizes is used to assess both weak and strong scaling features of the different implementations, more details in the **Results** section.

## Implementations
Let's discuss the different implementations I propose:
Suppose in the following that $$N$$ is the total number of processes.

### naive_gather.c
The first implementation is called `naive_gather` and works by having all processes but root issue a blocking send to root. Root on the other hand issues $$N$$ blocking receive. This is a simple linear approach with blocking communication.

The code can be summarized as:
```c
int* recv_buffer = NULL;
if (rank == 0) 
    recv_buffer = (int*)malloc(size * SEND_COUNT * sizeof(int));
int send_data[SEND_COUNT];
// [...]

int* curr_buffer = recv_buffer;
if (rank != 0)
    MPI_Send(send_data, SEND_COUNT, MPI_INT, 0, rank, MPI_COMM_WORLD);
else {
    for (int i=1; i<size; i++){ // Root calls one recv for each send
        curr_buffer += SEND_COUNT;  // Move buffer pointer along
        MPI_Recv(curr_buffer, SEND_COUNT, MPI_INT, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &status);
    }
}
```