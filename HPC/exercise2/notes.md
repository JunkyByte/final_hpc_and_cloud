I pick the gather operation for exercise 2a.

I start by writing a small benchmark using MPI_Gather default implementation.
I make a few assumptions on how this will work, let's briefly recap:

Objective: Implement a gather with p2p communication. In a gather communication
a root process receives data from all other processes. In my context I assume
that I want to share an array of integers of fixed size from all process to root one. The data must be ordered according to processes rank in the end.

Assume `np` is number of processes, at the end of the gather we want root process to have `np * N` integers, where `N` is the number of integers each process holds.

I assume that the amount of data each process has is the same and fixed and that the number of processes is >=2.

## Procedure

I incrementally build on top of previous iterations, trying to reach similar performance to the one of MPI_Gather by opting for different approaches.

### naive_gather.c
I start by implementing a Naive strategy to assess an initial baseline using p2p communication, a naive approach is that each process issues a blocking send to root. Root issues `np` blocking recv to receive all the data in order.

### naive_gather_improved.c
I try to build on top of this naive approach with a very similar one in which the recv are all issued together in a non-blocking fashion, then root waits for all communications to be done and proceeds.

### gather_ring.c
I create a ring communication in which each process sends
it's data to the left process until root receives all data.
this increases the number of communications but reduces the
each communication data size.
Note that the Recv are done in a blocking way and we wait for previous one
to be done.

### gather_binary_tree.c
The set of processes is conceptualized as a binary tree, where each process serves as a node. Beginning with the leaf nodes, each process shares its data with its parent, and this process continues until the root node is reached. As data moves up the tree, each node accumulates information from its children until the root receives the complete set of data from all processes.


## Scaling

With a gather operation I am a bit confused on how to assess scaling.

> Definition: Weak scaling measures how well a parallel algorithm performs as the problem size per processor or per core remains constant, but the overall problem size increases as more processors are added.

I assess weak scaling by having a certain amount of total data that must be gathered and changing the number of processes that share chunks of it.
Therefore if NP is number of processes
`data_size_per_process = max_data_size / NP` is the amount of data per process.

In strong scaling the amount of work should remain the same.

Definition: Strong scaling measures how well a parallel algorithm performs as the problem size remains constant, but the number of processors is varied.


TODO:

It would be interesting to build a Gather ring where there are multiple Rings performed in parallel
so the N processes are split in N/k actual chains each performing right to left ring communication. The most left node of each chain becomes the root process.