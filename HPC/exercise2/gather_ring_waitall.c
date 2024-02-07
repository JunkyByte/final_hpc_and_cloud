#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "const.h"


int main(int argc, char** argv) {

    // ****** INIT AND GENERATE DATA ******
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    const int dummy_size = 10;  // Size of the dummy data
    char dummy_data[dummy_size];

    // Warm-up phase
    for (int i = 0; i < 10; i++) {
        // Use MPI_Send/MPI_Recv with dummy data
        MPI_Send(dummy_data, dummy_size, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);
        MPI_Recv(dummy_data, dummy_size, MPI_CHAR, (rank + size - 1) % size, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }


    // The root process gathers data from all other processes
    int* recv_buffer = NULL;
    if (rank == 0) {
        // Allocate memory for the gathered data at the root
        recv_buffer = (int*)malloc(size * SEND_COUNT * sizeof(int));
        for (int k=0;k<SEND_COUNT;k++){
            if (SEND_COUNT < 128) // I use this for debugging
                recv_buffer[k] = k;
            else
                recv_buffer[k] = 0;
        }
    } else {
        // rank j need a buffer where they can store n - j messages
        recv_buffer = (int*)malloc((size - rank) * SEND_COUNT * sizeof(int));
    }

    int send_data[SEND_COUNT];
    for (int i=0; i<SEND_COUNT; i++){
        if (SEND_COUNT < 128) // I use this for debugging
            send_data[i] = rank * SEND_COUNT + i;
        else
            send_data[i] = rank;
    }
    // ************************************

    for (int i=0; i<SEND_COUNT; i++){
        recv_buffer[i] = send_data[i];
    }

    // Timing variables
    double start_time, end_time, delta;

    // The implementation
    // Start the timer
    // We want to end up with everything to root node
    // which we assume to be ID 0
    // We create a ring communication in which each process sends
    // it's data to the left process until root receives all data.

    // *** SETUP
    int* curr_buffer = recv_buffer;

    MPI_Request req;
    MPI_Status status;

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // We can use non blocking send and blocking receive.
    // each time we receive we can send to previous process.
    // root just waits for all communications
    // rank j receives 1 message and does 1 send
    // rank 0 receives 1 message and does 0 send
    curr_buffer += SEND_COUNT;

    if (rank != size - 1)
        MPI_Recv(curr_buffer, (size - rank - 1) * SEND_COUNT, MPI_INT, rank + 1, rank + 1, MPI_COMM_WORLD, &status);

    if (rank != 0){
        MPI_Send(recv_buffer, (size - rank) * SEND_COUNT, MPI_INT, rank - 1, rank, MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    delta = end_time - start_time;

    // TODO: Write test code that verifies gather is correct
    // if (rank == 0) {
    //      printf("Gathered data at the root process:\n");
    //      for (int i = 0; i < size * SEND_COUNT; ++i) {
    //          printf("%d ", recv_buffer[i]);
    //      }
    //      printf("\n");
    // }

    // free and print the time taken by the communication
    free(recv_buffer);
    if (rank == 0) {
        printf("%f\n", delta); // / REPETITIONS);
    }

    MPI_Finalize();
    return 0;
}
