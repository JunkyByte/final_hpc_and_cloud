#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "const.h"

int ROOT_ID = 0;


int main(int argc, char** argv) {

    // ****** INIT AND GENERATE DATA ******
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    // Warm-up phase
    const int dummy_size = 10;
    char dummy_data[dummy_size];
    for (int i = 0; i < 10; i++) {
        // Use MPI_Send/MPI_Recv with dummy data
        MPI_Send(dummy_data, dummy_size, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);
        MPI_Recv(dummy_data, dummy_size, MPI_CHAR, (rank + size - 1) % size, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }


    // The root process gathers data from all other processes
    int* recv_buffer = NULL;
    if (rank == ROOT_ID) {
        // Allocate memory for the gathered data at the root
        recv_buffer = (int*)malloc(size * SEND_COUNT * sizeof(int));
        for (int k=0;k<SEND_COUNT;k++){
            if (SEND_COUNT < 128) // I use this for debugging
                recv_buffer[k] = k;
            else
                recv_buffer[k] = ROOT_ID;
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

    // Timing variables
    double start_time, end_time, delta;
    MPI_Request req;

    MPI_Barrier(MPI_COMM_WORLD);
    // Start the timer
    start_time = MPI_Wtime();
    MPI_Gather(send_data, SEND_COUNT, MPI_INT, recv_buffer, SEND_COUNT, MPI_INT, ROOT_ID, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    delta = end_time - start_time;

    // TODO: Write test code that verifies gather is correct
    // if (rank == 0) {
    //     printf("Gathered data at the root process:\n");
    //     for (int i = 0; i < size * SEND_COUNT; ++i) {
    //         printf("%d ", recv_buffer[i]);
    //     }
    //     printf("\n");
    //     free(recv_buffer);  // Free the allocated memory
    // }

    // Print the time taken by the communication
    if (rank == ROOT_ID) {
        printf("%f\n", delta); // / REPETITIONS);
    }

    MPI_Finalize();
    return 0;
}
