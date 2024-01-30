#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include "const.h"


int count_descendants(int current_rank, int size) {
    if (current_rank < size) {
        return 1 + count_descendants(2 * current_rank + 1, size) + count_descendants(2 * current_rank + 2, size);
    } else {
        return 0;
    }
}

void preorder_order_iterative(int tree_size, int* argsort_result, int* index_to_argsort) {
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

    // Calculate argsort using an index array
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
        argsort_result[i] = index_to_argsort[i];
    }
}

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

    int tree_size = 7;
    int argsort_result[tree_size];
    int index_to_argsort[tree_size];
    preorder_order_iterative(tree_size, argsort_result, index_to_argsort);

    // We know use argsort to choose ranks that will lead to an ordered result.

    // Who's this process?
    int parent = (rank - 1) / 2;
    int left_child = 2 * rank + 1;
    int right_child = 2 * rank + 2;
    int num_children = (2 * rank + 2 < size) ? 2 : ((2 * rank + 1 < size) ? 1 : 0);
    int is_leaf = (2 * rank + 1 >= size && 2 * rank + 2 >= size) ? 1 : 0;
    int total_height = floor(log2(size + 1));
    int height = floor(log2(rank + 1));
    int total_descendants = (rank < size) ? count_descendants(2 * rank + 1, size) + count_descendants(2 * rank + 2, size) : 0;
    int left_descendants = (left_child < size) ? 1 + count_descendants(2 * left_child + 1, size) + count_descendants(2 * left_child + 2, size) : 0;
    int right_descendants = total_descendants - left_descendants;

    // The amount of total data the node holds and the amount it should receive
    size_t TOTAL_COUNT = (1 + total_descendants) * SEND_COUNT;
    size_t RECEIVE_COUNT_LEFT = left_descendants * SEND_COUNT;
    size_t RECEIVE_COUNT_RIGHT = right_descendants * SEND_COUNT;

    printf("I am %d and I have %d children, %d left, %d right\n", rank, total_descendants, left_descendants, right_descendants);
    printf("I am %d and I my buffer size %d left %d right %d\n", rank, TOTAL_COUNT, RECEIVE_COUNT_LEFT, RECEIVE_COUNT_RIGHT);

    // Allocate memory for the process and gathered data at each layer
    // Note that each node will have to receive a certain amount of data based on TOTAL number of descendants
    int* recv_buffer = (int*)malloc(TOTAL_COUNT * sizeof(int));

    int send_data[SEND_COUNT];
    for (int i=0; i<SEND_COUNT; i++){
        if (SEND_COUNT < 128) // I use this for debugging
            send_data[i] = rank * SEND_COUNT + i;
        else
            send_data[i] = rank;
    }

    // We copy the send data to the receive buffer (we could have populated it directly).
    // We will only send it after receiving anyway :)
    for (int i=0; i<SEND_COUNT; i++){
        recv_buffer[i] = send_data[i];
    }

    // ************************************

    // Timing variables
    double start_time, end_time, delta;

    // We want to end up with everything to root node
    // which we assume to be ID 0

    // *** SETUP
    int* curr_buffer = recv_buffer;

    MPI_Request req_receive[num_children];

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // We can use blocking send and blocking receive.
    // assuming complete tree (for simplicity in this discussion)
    // each non leaf node will Recv 2 times from its children.
    // then will perform a single send to its parent.
    // We use the variables defined above to know how many communications etc.
    // printf("I am %d and my height is %d out of %d\n", rank, height, total_height);

    // All non leaf nodes will issue their receive operations
    // Note that the amount of data differs between layers of the tree
    if (left_child < size){
        curr_buffer += SEND_COUNT;
        MPI_Irecv(curr_buffer, RECEIVE_COUNT_LEFT, MPI_INT, left_child, 0, MPI_COMM_WORLD, &req_receive[0]);
    }
    if (right_child < size){
        curr_buffer += RECEIVE_COUNT_LEFT;
        MPI_Irecv(curr_buffer, RECEIVE_COUNT_RIGHT, MPI_INT, right_child, 0, MPI_COMM_WORLD, &req_receive[1]);
    }

    if (num_children)
        MPI_Waitall(num_children, req_receive, MPI_STATUS_IGNORE);

    // All process also send data, here non leaf will have already received their part while leaf nodes
    // are always free to send

    // Everybody does a send but root!
    if (rank != 0){
        // Does a single send! It can be blocking because once it is done the work is finished.
        MPI_Send(recv_buffer, TOTAL_COUNT, MPI_INT, parent, 0, MPI_COMM_WORLD);
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
