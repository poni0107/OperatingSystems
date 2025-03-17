#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define MAX_THREADS 8 // Maksimalni broj niti

// Struktura koja sadrži podatke koje će nit koristiti za množenje
typedef struct {
    int thread_id;
} ThreadData;

int m = 5; // Broj redova prve matrice
int n = 5; // Broj kolona prve matrice
int k = 5; // Broj kolona druge matrice

int** matrix_a;
int** matrix_b;
int** result_matrix;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int global_accumulator = 0;


void* thread_function(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    
    int start_row = data->thread_id * (m / MAX_THREADS);
    int end_row = (data->thread_id == MAX_THREADS - 1) ? m : (data->thread_id + 1) * (m / MAX_THREADS);

    
    for (int i = start_row; i < end_row; i++) {
        int local_accumulator = 0;
        for (int j = 0; j < n; j++) {
            for (int l = 0; l < k; l++) {
                // lokalna varijabla u kojoj se akumulira rezultat mnozenja jedne vriste i jedne kolone
                local_accumulator += matrix_a[i][j] * matrix_b[j][l];
            }
        }
        
        pthread_mutex_lock(&mutex);
        global_accumulator += local_accumulator;
      
        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}

int main() {
    
    
    matrix_a = (int**)malloc(m * sizeof(int*));
    for (int i = 0; i < m; i++) {
        matrix_a[i] = (int*)malloc(n * sizeof(int));
        for (int j = 0; j < n; j++) {
            matrix_a[i][j] = i * n + j + 1 /* dodeljivanje vrednosti */;
        }
    }

    
    matrix_b = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        matrix_b[i] = (int*)malloc(k * sizeof(int));
        for (int j = 0; j < k; j++) {
            matrix_b[i][j] = i * n + j + 1 /* dodeljivanje vrednosti */;
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
            printf("%d ", matrix_b[i][j]);
        }
        printf("\n");
    }


    
    result_matrix = (int**)malloc(m * sizeof(int*));
    for (int i = 0; i < m; i++) {
        result_matrix[i] = (int*)malloc(k * sizeof(int));
    }

    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];
    
    struct timeval start_time, end_time;
    double elapsed_time;

    gettimeofday(&start_time, NULL); // Početak merenja vremena

    
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_data[i].thread_id = i;
        pthread_create(&threads[i], NULL, thread_function, (void*)&thread_data[i]);
    }

    
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end_time, NULL); // Kraj merenja vremena
    elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    // Ispis rezultata i vremena izvršavanja
    printf("Global Accumulator: %d\n", global_accumulator);
    printf("Elapsed Time: %.6f seconds\n", elapsed_time);

    // Oslobađanje memorije
    for (int i = 0; i < m; i++) {
        free(result_matrix[i]);
    }
    free(result_matrix);

    return 0;
}
