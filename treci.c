#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define MAX_THREADS 4 // Maksimalni broj niti


typedef struct {
    int thread_id;       // ID niti
    int rows;            // Broj redova matrice
    int cols;            // Broj kolona matrice
    int** matrix_a;      // Matrica A
    int** matrix_b;      // Matrica B
    int** result_matrix; 
} ThreadData;

int start_row = data->thread_id * (data->rows / MAX_THREADS);
    int end_row = (data->thread_id == MAX_THREADS - 1) ? data->rows : (data->thread_id + 1) * (data->rows / MAX_THREADS);

void* thread_function(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    for (int i = start_row; i < end_row; i++) {
        for (int j = 0; j < data->cols; j++) {
            data->result_matrix[i][j] = data->matrix_a[i][j] + data->matrix_b[i][j];
        }
    }


    pthread_exit(NULL);
}

int main() {
    int m = 5; // Broj redova matrice
    int n = 5; // Broj kolona matrice

    // Inicijalizacija matrica A i B
    int** matrix_a = (int**)malloc(m * sizeof(int*));
    int** matrix_b = (int**)malloc(m * sizeof(int*));

    // Popunjavanje matrica A i B vrednostima
    for (int i = 0; i < m; i++) {
        matrix_a[i] = (int*)malloc(n * sizeof(int));
        matrix_b[i] = (int*)malloc(n * sizeof(int));
        
        // Popunjavanje matrica vrednostima
        for (int j = 0; j < n; j++) {
            matrix_a[i][j] = i * n + j + 1; 
            matrix_b[i][j] = i * n + j + 1; // Matrice a i be ce biti identicne, ove vrednosti se mogu menjati
        }
    }

    printf("Matrix A:\n");
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", matrix_a[i][j]);
        }
        printf("\n");
    }

    printf("Matrix B:\n");
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", matrix_a[i][j]);
        }
        printf("\n");
    }

    // Inicijalizacija rezultujuće matrice
    int** result_matrix = (int**)malloc(m * sizeof(int*));
    for (int i = 0; i < m; i++) {
        result_matrix[i] = (int*)malloc(n * sizeof(int));
    }


    pthread_t threads[MAX_THREADS];     
    ThreadData thread_data[MAX_THREADS]; 

    struct timeval start_time, end_time; 
    double elapsed_time;                 

    gettimeofday(&start_time, NULL);     

    for (int i = 0; i < MAX_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].rows = m;
        thread_data[i].cols = n;
        thread_data[i].matrix_a = matrix_a;
        thread_data[i].matrix_b = matrix_b;
        thread_data[i].result_matrix = result_matrix;
        pthread_create(&threads[i], NULL, thread_function, (void*)&thread_data[i]); 
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        
        pthread_join(threads[i], NULL);
    }

   
    gettimeofday(&end_time, NULL);

    elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    // Ispis rezultujuće matrice
    printf("Result Matrix:\n");
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", result_matrix[i][j]);
        }
        printf("\n");
    }

    // Ispis vremena izvršavanja
    printf("Elapsed Time: %.6f seconds\n", elapsed_time);

    // Oslobađanje memorije
    for (int i = 0; i < m; i++) {
        free(matrix_a[i]);
        free(matrix_b[i]);
        free(result_matrix[i]);
    }
    free(matrix_b);
    free(matrix_a);
    free(result_matrix);

    return 0;
}
