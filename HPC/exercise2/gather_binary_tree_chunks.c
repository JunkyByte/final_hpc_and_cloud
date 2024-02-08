#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include "const.h"

#define CHUNK_SIZE 1  // TODO

int count_descendants(int current_rank, int size) {
    if (current_rank < size) {
        return 1 + count_descendants(2 * current_rank + 1, size) + count_descendants(2 * current_rank + 2, size);
    } else {
        return 0;
    }
}

// Function to perform preorder traversal and build the argsort dictionary
void preorder_order_iterative(int tree_size, int* rank_to_tree_rank, int* tree_rank_to_rank) {
    // Create an array to represent the preorder traversal
    int preorder_result[tree_size];

    int stack[tree_size];
    int top = -1;

    int result_index = 0;

    stack[++top] = 0; // Start with the root index (assuming the root index is 0)

    while (top >= 0) {
        int node = stack[top--];
        preorder_result[result_index++] = node;

        // Push the right child first so that it is processed after the left child
        if (2 * node + 2 < tree_size) {
            stack[++top] = 2 * node + 2;
        }

        if (2 * node + 1 < tree_size) {
            stack[++top] = 2 * node + 1;
        }
    }

    // Calculate argsort directly while building the index_to_argsort array
    int index_to_argsort[tree_size];
    for (int i = 0; i < tree_size; ++i) {
        index_to_argsort[i] = i;
    }

    for (int i = 0; i < tree_size; ++i) {
        for (int j = i + 1; j < tree_size; ++j) {
            if (preorder_result[index_to_argsort[i]] > preorder_result[index_to_argsort[j]]) {
                // Swap indices if out of order
                int temp = index_to_argsort[i];
                index_to_argsort[i] = index_to_argsort[j];
                index_to_argsort[j] = temp;
            }
        }
    }

    // Copy the argsort order to the result array
    for (int i = 0; i < tree_size; ++i) {
        int j;
        for (j = 0; j < tree_size; ++j){
            if (index_to_argsort[j] == i)
                break;
        }
        rank_to_tree_rank[i] = j;
        tree_rank_to_rank[j] = i;
        // rank_to_tree_rank[i] = index_to_argsort[i];
    }
}


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int dummy_size = 10;
    char dummy_data[dummy_size];

    for (int i = 0; i < 10; i++) {
        MPI_Send(dummy_data, dummy_size, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);
        MPI_Recv(dummy_data, dummy_size, MPI_CHAR, (rank + size - 1) % size, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    int rank_to_tree_rank[size];
    int tree_rank_to_rank[size];
    preorder_order_iterative(size, rank_to_tree_rank, tree_rank_to_rank);

    int tree_rank = rank_to_tree_rank[rank];
    int parent = (tree_rank - 1) / 2;
    int parent_rank = tree_rank_to_rank[parent];
    int left_child = 2 * tree_rank + 1;
    int left_child_rank = (left_child < size) ? tree_rank_to_rank[left_child] : -1;
    int right_child = 2 * tree_rank + 2;
    int right_child_rank = (right_child < size) ? tree_rank_to_rank[right_child] : -1;
    int num_children = (2 * tree_rank + 2 < size) ? 2 : ((2 * tree_rank + 1 < size) ? 1 : 0);
    int total_descendants = (tree_rank < size) ? count_descendants(2 * tree_rank + 1, size) + count_descendants(2 * tree_rank + 2, size) : 0;
    int left_descendants = (left_child < size) ? 1 + count_descendants(2 * left_child + 1, size) + count_descendants(2 * left_child + 2, size) : 0;
    int right_descendants = total_descendants - left_descendants;

    size_t TOTAL_COUNT = (1 + total_descendants) * CHUNK_SIZE;
    size_t RECEIVE_COUNT_LEFT = left_descendants * CHUNK_SIZE;
    size_t RECEIVE_COUNT_RIGHT = right_descendants * CHUNK_SIZE;

    int* recv_buffer = NULL; 
    int* tmp_buffer = NULL; 
    if (rank == 0){ // Root needs space for whole data and one to receive messages
        recv_buffer = (int*) malloc((1 + total_descendants) * SEND_COUNT * sizeof(int));
        tmp_buffer = (int*) malloc(total_descendants * CHUNK_SIZE * sizeof(int));
    } else // Every other rank need space for 1 chunk
        recv_buffer = (int*) malloc(TOTAL_COUNT * sizeof(int));

    int send_data[SEND_COUNT];
    for (int i = 0; i < SEND_COUNT; i++) {
        if (SEND_COUNT < 128)
            send_data[i] = rank * SEND_COUNT + i;
        else
            send_data[i] = rank;
    }
    
    if (rank == 0){
        for (int i=0; i<SEND_COUNT; i++){
            recv_buffer[i] = send_data[i];
        }
    }

    int TOTAL_CHUNKS = SEND_COUNT / CHUNK_SIZE;

    // ************************************

    // Timing variables
    double start_time, end_time, delta;

    // We want to end up with everything to root node
    // which we assume to be ID 0

    MPI_Request req_receive[num_children];

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // Split the data into chunks and perform the gather operation on each chunk
    if (rank != 0){
        for (int chunk = 0; chunk < TOTAL_CHUNKS; chunk++) {
            for (int i = 0; i < CHUNK_SIZE; i++) {
                recv_buffer[i] = send_data[chunk * CHUNK_SIZE + i];
                printf("Copied to send: %d\n", send_data[chunk * CHUNK_SIZE + i]);
            }

            if (left_child < size) {
                printf("CHILD Receive left %d\n", chunk);
                MPI_Irecv(recv_buffer + CHUNK_SIZE, RECEIVE_COUNT_LEFT, MPI_INT, left_child_rank, chunk, MPI_COMM_WORLD, &req_receive[0]);
                if (right_child < size)
                    MPI_Irecv(recv_buffer + CHUNK_SIZE + RECEIVE_COUNT_LEFT, RECEIVE_COUNT_RIGHT, MPI_INT, right_child_rank, chunk, MPI_COMM_WORLD, &req_receive[1]);
            }

            if (num_children) {
                MPI_Waitall(num_children, req_receive, MPI_STATUSES_IGNORE);
            }

            printf("CHILD Sending %d\n", chunk);
            MPI_Send(recv_buffer, TOTAL_COUNT, MPI_INT, parent_rank, chunk, MPI_COMM_WORLD);
        }
    } else {
        for (int chunk = 0; chunk < TOTAL_CHUNKS; chunk++) {
            if (left_child < size) {
                printf("CHILD Receive left %d\n", chunk);
                MPI_Irecv(tmp_buffer, RECEIVE_COUNT_LEFT, MPI_INT, left_child_rank, chunk, MPI_COMM_WORLD, &req_receive[0]);
                if (right_child < size)
                    MPI_Irecv(tmp_buffer + RECEIVE_COUNT_LEFT, RECEIVE_COUNT_RIGHT, MPI_INT, right_child_rank, chunk, MPI_COMM_WORLD, &req_receive[1]);
            }

            if (num_children) {
                MPI_Waitall(num_children, req_receive, MPI_STATUSES_IGNORE);
            }

            printf("Received %d\n", chunk);

            // I need to copy data from tmp_buffer to recv_buffer in the correct place
            for (int i=0; i<total_descendants * CHUNK_SIZE; i++){
                printf("%d ", tmp_buffer[i]);
                recv_buffer[SEND_COUNT + chunk * CHUNK_SIZE + i * TOTAL_CHUNKS] = tmp_buffer[i];
            }
            printf("\n");
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    delta = end_time - start_time;

    // TODO: Write test code that verifies gather is correct
    if (rank == 0) {
         printf("Gathered data at the root process:\n");
         for (int i = 0; i < size * SEND_COUNT; ++i) {
             printf("%d ", recv_buffer[i]);
         }
         printf("\n");
    }

    // free and print the time taken by the communication
    free(recv_buffer);
    if (rank == 0) {
        printf("%f\n", delta); // / REPETITIONS);
    }

    MPI_Finalize();
    return 0;
}
