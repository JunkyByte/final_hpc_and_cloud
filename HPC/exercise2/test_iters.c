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
    }

    int send_data[SEND_COUNT];
    for (int i=0; i<SEND_COUNT; i++){
        if (SEND_COUNT < 128) // I use this for debugging
            send_data[i] = rank * SEND_COUNT + i;
        else
            send_data[i] = rank;
    }
    // ************************************

    MPI_Status status;

    // Timing variables
    double start_time, end_time, delta;

    // The implementation
    // Naive approach with blocking calls as baseline
    // Each process but root call a send passing its rank as tag
    // The rank is used only to receive the data in order

    // *** SETUP
    // Start the timer
    for (int k=0; k<REPETITIONS;k++){
        int* curr_buffer = recv_buffer;
        MPI_Barrier(MPI_COMM_WORLD);
        start_time = MPI_Wtime();

        if (rank != 0){
            MPI_Send(send_data, SEND_COUNT, MPI_INT, 0, rank, MPI_COMM_WORLD);
        } else {
            // Root calls one recv for each send
            for (int i=1; i<size; i++){
                curr_buffer += SEND_COUNT;  // Move buffer pointer along
                MPI_Recv(curr_buffer, SEND_COUNT, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            }
        }

        end_time = MPI_Wtime();
        delta += end_time - start_time;
    }

    // TODO: Write test code that verifies gather is correct
    // if (rank == 0) {
    //     printf("Gathered data at the root process:\n");
    //     for (int i = 0; i < size * SEND_COUNT; ++i) {
    //         printf("%d ", recv_buffer[i]);
    //     }
    //     printf("\n");
    // }

    // free and print the time taken by the communication
    if (rank == 0) {
        free(recv_buffer);
        printf("%f\n", delta / REPETITIONS);
    }

    MPI_Finalize();
    return 0;
}
