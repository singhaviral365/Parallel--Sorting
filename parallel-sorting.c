#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define MASTER_RANK 0

// Function to swap two elements in an array
void swap(int* arr, int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// Function to perform quicksort on a portion of the array
void quicksort(int* arr, int low, int high) {
    if (low < high) {
        int pivot = arr[high];
        int i = low - 1;

        for (int j = low; j <= high - 1; j++) {
            if (arr[j] < pivot) {
                i++;
                swap(arr, i, j);
            }
        }
        swap(arr, i + 1, high);
        int pivot_index = i + 1;

        quicksort(arr, low, pivot_index - 1);
        quicksort(arr, pivot_index + 1, high);
    }
}

// Function to merge two sorted arrays
void merge(int* arr, int* temp, int low, int mid, int high) {
    int i = low;
    int j = mid + 1;
    int k = low;

    while (i <= mid && j <= high) {
        if (arr[i] <= arr[j]) {
            temp[k++] = arr[i++];
        } else {
            temp[k++] = arr[j++];
        }
    }

    while (i <= mid) {
        temp[k++] = arr[i++];
    }

    while (j <= high) {
        temp[k++] = arr[j++];
    }

    for (i = low; i <= high; i++) {
        arr[i] = temp[i];
    }
}

// Function to perform parallel merge sort
void parallelMergeSort(int* arr, int* temp, int low, int high) {
    if (low < high) {
        int mid = (low + high) / 2;

        // Recursively sort the two halves
        parallelMergeSort(arr, temp, low, mid);
        parallelMergeSort(arr, temp, mid + 1, high);

        // Merge the sorted halves
        merge(arr, temp, low, mid, high);
    }
}

int main(int argc, char* argv[]) {
    int rank, num_procs;
    int* arr;
    int* temp;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    int size;

    if (rank == MASTER_RANK) {
        // Prompt the user for the array size
        printf("Enter the size of the array: ");
        scanf("%d", &size);

        // Allocate memory for the array and temporary storage
        arr = (int*)malloc(size * sizeof(int));
        temp = (int*)malloc(size * sizeof(int));

        // Prompt the user for the array elements
        printf("Enter %d integers for the array:\n", size);
        for (int i = 0; i < size; i++) {
            scanf("%d", &arr[i]);
        }
    }

    // Broadcast the array size to all processes
    MPI_Bcast(&size, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    if (rank != MASTER_RANK) {
        // Allocate memory for the array and temporary storage in other processes
        arr = (int*)malloc(size * sizeof(int));
        temp = (int*)malloc(size * sizeof(int));
    }

    // Broadcast the unsorted array to all processes
    MPI_Bcast(arr, size, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    // Divide the array into chunks for parallel sorting
    int chunk_size = size / num_procs;
    int* local_arr = (int*)malloc(chunk_size * sizeof(int));

    // Scatter the chunks of the array to different processes
    MPI_Scatter(arr, chunk_size, MPI_INT, local_arr, chunk_size, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    // Perform quicksort on the local chunks
    quicksort(local_arr, 0, chunk_size - 1);

    // Gather the sorted chunks back to the master process
    MPI_Gather(local_arr, chunk_size, MPI_INT, arr, chunk_size, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    // Master process performs the final merge sort
    if (rank == MASTER_RANK) {
        parallelMergeSort(arr, temp, 0, size - 1);
    }

    MPI_Finalize();

    if (rank == MASTER_RANK) {
        // Print the sorted array (only from the master process)
        printf("Sorted array:\n");
        for (int i = 0; i < size; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
    }

    // Free dynamically allocated memory
    free(arr);
    free(temp);
    free(local_arr);

    return 0;
}
