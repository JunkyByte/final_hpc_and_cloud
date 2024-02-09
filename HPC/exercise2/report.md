# Final Project HPC - Exercise 2A
# Gather operation using p2p communication.

- Adriano Donninelli adonnine@sissa.it

## Introduction

I have chosen to implement a custom version of the Gather collective operation using point-to-point (p2p) communication in the context of MPI (Message Passing Interface). The gather operation involves the root process collecting data from all other processes. For semplicity I aim to share an array of integers of fixed size from all processes to the root process. Extending the implementations I provide to support tags, custom group communicators and arbitrary data types its quite trivial as p2p communication support all these features but was left out for simplicity as we pose more attention on the timings of the different solutions.

I will compare my custom gather operations with the different algorithms of the MPI_Gather implementation provided by OpenMPI. For simplicity, in some of the scripts I will assume that root process is rank 0.

## Gather operation

Before proceding let's define how the Gather collective operation works:

Suppose that there are `N` processes and that each has a fixed amount of data with size `K`.
Performing a gather operations means that one particular process, denoted as root receives from all the other processes their chunk of data. At the end of a gather operation the root process will hold a buffer of data of size `N * K`.
The gathered data must be ordered as the ranks assigned to the processes.

![gather_fig](https://i.imgur.com/efTku4k.png)

## Experimental Setup

In order to obtain more consistent timings I perform multiple runs of all the algorithms and average the timings, I also perform a warmup of the communication channels passing dummy data between the processes. For each solution I only time the actual gather operation and not the variable setup required by different algorithms. To perform all the timings I write a set of bash scripts which allow to perform weak and strong scaling measurements of the different implementations. The tests are performed on ORFEO cluster using the epyc partition. A varying amount of nodes and data sizes is used to assess both weak and strong scaling features of the different implementations, more details in the **Results** section.

## Implementations
Let's discuss the different implementations I propose:
Suppose in the following that `N` is the total number of processes.

### naive_gather.c
The first implementation is called `naive_gather` and works by having all processes but root issue a blocking send to root. Root on the other hand issues `N` blocking receive. This is a simple linear approach with blocking communication.

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
    for (int i=1; i<size; i++){
        curr_buffer += SEND_COUNT;  // Move buffer pointer along
        MPI_Recv(curr_buffer, SEND_COUNT, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }
}
```

In this implementation the root process receives the requests in order, a simple variant (`naive_gather_improved.c`) in which I non-blocking receive operations are used is also provided.
I imagined that using non blocking receive would improve results but it proved only marginally effective.

### gather_ring.c

I experimented with a different communication pattern between the processes. Assuming root is rank 0, I create a ring communication in which each process sends current data to left process until root receives all the data. (The name gather_pipeline would have been a more sensible choice). This pattern is not very efficient but is a good exercise.

![ring_fig](https://i.imgur.com/ENxdRtM.png)

This way rank `j` receives `N - j - 1` messages and does `N - j` send.
Rank `0` receives `N - 1` messages and does `0` send.

The code can be summarized as:
```c
int* recv_buffer = NULL;
if (rank == 0) {
    recv_buffer = (int*)malloc(size * SEND_COUNT * sizeof(int));
} else
    recv_buffer = (int*)malloc(SEND_COUNT * sizeof(int));  // 1 message buffer
int send_data[SEND_COUNT];
[...]

int* curr_buffer = recv_buffer;
MPI_Request req;
MPI_Status status;

if (rank != 0){
    MPI_Send(send_data, SEND_COUNT, MPI_INT, rank - 1, rank, MPI_COMM_WORLD);
    for (int i=0; i<size-rank-1; i++){
        MPI_Recv(recv_buffer, SEND_COUNT, MPI_INT,
                 rank + 1, rank + 1, MPI_COMM_WORLD, &status);
        MPI_Send(recv_buffer, SEND_COUNT, MPI_INT, rank - 1, rank, MPI_COMM_WORLD);
    }
} else {
    for (int i=0; i<size - 1; i++){ // Root receives all communications
        curr_buffer += SEND_COUNT;  // Move buffer pointer along
        MPI_Recv(curr_buffer, SEND_COUNT, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
    }
}
```

I thought of using buffered non blocking send in order to reuse the buffer and issue a recv before the previous send finishes but while it worked for me locally I received an error on ORFEO. It might be that buffered send are disabled or I am making some basic mistake. `[epyc003] pml_ucx.c:743  Error: bsend: failed to allocate buffer`.

Anyway, the idea was to use the following loop for `rank != 0`:
```c
MPI_Request req;
MPI_Isend(send_data, SEND_COUNT, MPI_INT, rank - 1, rank, MPI_COMM_WORLD, &req);
for (int i=0; i<size-rank-1; i++){
    MPI_Recv(recv_buffer, SEND_COUNT, MPI_INT, rank + 1, rank + 1, MPI_COMM_WORLD, &status);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MPI_Ibsend(recv_buffer, SEND_COUNT, MPI_INT, rank - 1, rank, MPI_COMM_WORLD, &req);
}
MPI_Wait(&req, MPI_STATUS_IGNORE);
```
I would expect a better performance with this buffered approach as the next receive operation can be issued before the previous send has finished. In this case a buffered send is required because otherwise the `recv_buffer` could be overwritten before it is safe to do so, causing undefined behaviour.

### gather_ring_waitall.c

I propose an even more inefficient variant of the previous approach where the `j-th` process accumulates all the data coming from its right neighbors before sending it to its left.

While inefficient the implementation is quite elegant, with a single send and receive for all but first and last ranks.
```c
if (rank != size - 1)
    MPI_Recv(curr_buffer, (size - rank - 1) * SEND_COUNT, MPI_INT,
             rank + 1, rank + 1, MPI_COMM_WORLD, &status);

if (rank != 0)
    MPI_Send(recv_buffer, (size - rank) * SEND_COUNT, MPI_INT,
             rank - 1, rank, MPI_COMM_WORLD);
```
This approach won't be included in the results as it infeasible.

### gather_binary_tree.c

The last implementation pattern I propose is based on the usage of a binary tree pattern. The set of processes is conceptualized as a binary tree, where each process serves as a node. Beginning with the leaf nodes, each process shares the data with its parent, and this process continues until the root node is reached. As data moves up the tree, each node accumulates data from its children until the root receives it.

![binary_comm](https://i.imgur.com/ylSXiRj.png)
> The communications with the same color do not depend on each other and are issued in parallel.

This implementation required more effort because the binary tree pattern is more complex but also because the ranks given by openMPI can't be used directly as tree ranks otherwise the data collected by root will not be ordered.

By observing the way I accumulate data in intermediate buffers along the tree and how the processes are ordered I noticed that I needed to traverse the tree following a preorder traversal pattern to have the data naturally ordered at root. By knowing this I rearranged the ranks of the tree starting from the mpi ranks to gather data correctly.

I write here only a part of the code as it is quite long. A few variables are computed regarding the tree structure:
```c
// [...] Computed variables regarding tree structure
// Calculate amount of total data the node holds and the amount it should receive
size_t TOTAL_COUNT = (1 + total_descendants) * SEND_COUNT;
size_t RECEIVE_COUNT_LEFT = left_descendants * SEND_COUNT;
size_t RECEIVE_COUNT_RIGHT = right_descendants * SEND_COUNT;

// Each node will have to receive a certain amount of data based on TOTAL number of descendants
int* recv_buffer = (int*)malloc(TOTAL_COUNT * sizeof(int));

// [...] More convenient to copy the send data to the receive buffer.
for (int i=0; i<SEND_COUNT; i++)
    recv_buffer[i] = send_data[i];

int* curr_buffer = recv_buffer;
MPI_Request req_receive[num_children];

// All non leaf nodes will issue their receive operations
// the amount of data differs between layers of the tree
if (left_child < size){
    curr_buffer += SEND_COUNT;
    MPI_Irecv(curr_buffer, RECEIVE_COUNT_LEFT, MPI_INT, left_child_rank,
              0, MPI_COMM_WORLD, &req_receive[0]);
    if (right_child < size){
        curr_buffer += RECEIVE_COUNT_LEFT;
        MPI_Irecv(curr_buffer, RECEIVE_COUNT_RIGHT, MPI_INT, right_child_rank,
                  0, MPI_COMM_WORLD, &req_receive[1]);
    }
}

if (num_children) // We wait to receive data from children
    MPI_Waitall(num_children, req_receive, MPI_STATUS_IGNORE);

// Everybody does a send but root!
if (rank != 0){
    // Single send! It can be blocking because once it is done the work is finished.
    MPI_Send(recv_buffer, TOTAL_COUNT, MPI_INT, parent_rank, 0, MPI_COMM_WORLD);
}
```

I also provide a version of the binary tree (`gather_binary_tree_chunks.c`) with message splitting into chunks of fixed size. The code is quite similar but involves some more complex buffer manipulations. The binary tree is executed multiple times sharing different parts of the message.

## Results

TODO








