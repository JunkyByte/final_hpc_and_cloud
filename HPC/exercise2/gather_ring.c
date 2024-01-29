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


    const int dummy_size = 100;  // Size of the dummy data
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
        for (int k=0;k<SEND_COUNT;k++)
            recv_buffer[k] = 0;
    } else {
        // All ranks need a buffer where they can store rank + 1 message
        recv_buffer = (int*)malloc(SEND_COUNT * sizeof(int));
    }

    int send_data[SEND_COUNT];
    for (int i=0; i<SEND_COUNT; i++){
        send_data[i] = rank;
    }
    // ************************************

    // Timing variables
    double start_time, end_time, delta;

    // The implementation
    // Start the timer
    // We want to end up with everything to root node
    // which we assume to be ID 0
    // We create a ring communication in which each process sends
    // it's data to the left process until root receives all data.
    // this increases the number of communications but reduces the
    // each communication data size

    // *** SETUP
    int* curr_buffer = recv_buffer;

    MPI_Request req;
    MPI_Status status;

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // We can use non blocking send and blocking receive.
    // each time we receive we can send to previous process.
    // root just waits for all communications
    // rank j receives n - j - 1 messages and does n - j send
    // rank 0 receives n - 1 messages and does 0 send
    if (rank != 0){
        // Start by sending your own data
        // printf("I am %d and I'm sending my initial data\n", rank);
        MPI_Isend(send_data, SEND_COUNT, MPI_INT, rank - 1, rank, MPI_COMM_WORLD, &req);
        for (int i=0; i<size-rank-1; i++){
            // Perform all other receive and send
            // printf("I am %d and I'm waiting to receive my %d communication\n", rank, i + 1);
            MPI_Recv(recv_buffer, SEND_COUNT, MPI_INT, MPI_ANY_SOURCE, rank + 1, MPI_COMM_WORLD, &status);
            // TODO: Here for simplicity I wait for previous communications to be done
            // multiple sends could be done in parallel by same process
            // but then I should check if they are actually received in order.
            // also the recv buffer should then be processed differently
            // we could do this by using tags to indicate the current communication so that
            // we receive in order and use bufferend non blocking send
            MPI_Wait(&req, MPI_STATUS_IGNORE);  // I do not need to get the status
            MPI_Isend(recv_buffer, SEND_COUNT, MPI_INT, rank - 1, rank, MPI_COMM_WORLD, &req);
            // printf("I am %d and I'm sending my %d communication\n", rank, i + 1);
        }
        MPI_Wait(&req, MPI_STATUS_IGNORE);  // I do not need to get the status
    } else {
        // Issue Irecv
        for (int i=0; i<size - 1; i++){
            // printf(">>> I am %d and I'm waiting to receive my %d communication\n", rank, i + 1);
            curr_buffer += SEND_COUNT;  // Move buffer pointer along
            MPI_Recv(curr_buffer, SEND_COUNT, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
        }
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
