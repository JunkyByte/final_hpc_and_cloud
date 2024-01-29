#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int ROOT_ID = 0;

// Number of repetitions for each simulation
int REPETITIONS = 100;

// Number of elements to be sent by each process
// int is 4 bytes so 4 * send_count data for each process
int SEND_COUNT = 32768;


int main(int argc, char** argv) {

    // ****** INIT AND GENERATE DATA ******
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    // The root process gathers data from all other processes
    int* recv_buffer = NULL;
    if (rank == 0) {
        // Allocate memory for the gathered data at the root
        recv_buffer = (int*)malloc(size * SEND_COUNT * sizeof(int));
    }

    int send_data[SEND_COUNT];
    for (int i=0; i<SEND_COUNT; i++){
        send_data[i] = rank;
    }
    // ************************************

    // Timing variables
    double start_time, end_time, delta;

    // The implementation
    for (int k=0; k<REPETITIONS; k++){
        // Naive approach with parallel recv calls
        // Each process but root call a send passing its rank as tag
        // The rank is used only to receive the data in order

        // *** SETUP
        int* curr_buffer = recv_buffer;

        // Create a vector of MPI_Requests to hold requests.
        MPI_Request reqs[size];

        // Start the timer
        start_time = MPI_Wtime();

        if (rank != 0){
            MPI_Send(send_data, SEND_COUNT, MPI_INT, ROOT_ID, rank, MPI_COMM_WORLD);
        } else {

            // Issue Irecv
            for (int i=1; i<size; i++){
                curr_buffer += SEND_COUNT;  // Move buffer pointer along
                MPI_Irecv(curr_buffer, SEND_COUNT, MPI_INT, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &reqs[i]);
            }

            // Wait all requests to be done
            for (int i=1; i<size; i++)
                MPI_Wait(&reqs[i], MPI_STATUS_IGNORE);  // I do not need to get the status
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
        printf("Time taken by naive_gather: %f seconds\n", delta); // / REPETITIONS);
    }

    MPI_Finalize();
    return 0;
}
