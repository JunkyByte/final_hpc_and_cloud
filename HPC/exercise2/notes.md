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