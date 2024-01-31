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
        // All ranks need a buffer where they can store size - rank - 1 messages
        recv_buffer = (int*)malloc((size - rank - 1) * SEND_COUNT * sizeof(int));
    }

    int send_data[SEND_COUNT];
    for (int i=0; i<SEND_COUNT; i++){
        if (SEND_COUNT < 128) // I use this for debugging
            send_data[i] = rank * SEND_COUNT + i;
        else
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

    MPI_Request reqs[size - rank - 1];
    MPI_Request reqs_send[size - rank];
    MPI_Status status;

    int count_done = 0;
    int done[size-rank-1];
    for (int i=0;i<size-rank-1;i++){
        done[i] = 0;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // root just waits for all communications
    // rank j receives n - j - 1 messages and does n - j send
    // rank 0 receives n - 1 messages and does 0 send
    if (rank != 0){
        // Start by sending your own data
        // printf("I am %d and I'm sending my initial data\n", rank);
        MPI_Isend(send_data, SEND_COUNT, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &reqs_send[0]);

        // We issue all future receive operations in parallel, populating the reqs
        for (int i=0; i<size-rank-1; i++){
            MPI_Irecv(curr_buffer, SEND_COUNT, MPI_INT, rank + 1, i, MPI_COMM_WORLD, &reqs[i]);
            curr_buffer += SEND_COUNT;  // Move buffer pointer along
        }

        // Then we issue all our send in parallel as soon as the data is ready
        // we test the reqs to be sure it is
        while (1){
            for (int i=0; i<size-rank-1; i++){
                if (done[i])
                    continue;

                MPI_Test(&reqs[i], &done[i], MPI_STATUS_IGNORE);  // I do not need to get the status
                if (done[i]){
                    // The i-th data block arrived
                    // printf("I am %d and I received my %d data, I can now issue a send!\n", rank, i + 1);
                    MPI_Isend(recv_buffer + i * SEND_COUNT, SEND_COUNT, MPI_INT, rank - 1, i + 1, MPI_COMM_WORLD, &reqs_send[i + 1]);
                    ++count_done;
                }
            }

            if (count_done == size - rank - 1)
                break;
        }
        // printf("I am %d and I issued all my sends!\n", rank);
    } else {
        // Issue all Irecv
        for (int i=0; i<size - 1; i++){
            curr_buffer += SEND_COUNT;  // Move buffer pointer along
            MPI_Irecv(curr_buffer, SEND_COUNT, MPI_INT, 1, i, MPI_COMM_WORLD, &reqs[i]);
        }

        MPI_Waitall(size - rank - 1, reqs, MPI_STATUS_IGNORE);
        // printf(">>> I am ROOT and received all my communications\n");
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
