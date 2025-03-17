#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_THREADS 8 // Maksimalni broj niti

typedef struct {
    int thread_id;
    int d;              // Broj niti za rotaciju redova
    int s;              // Broj rotacija koje će svaka nit izvršiti
    int m;              // Broj redova i kolona matrice
    int** matrix;       // Matrica koja se rotira
    pthread_mutex_t* mutex;     // Mutex za sinhronizaciju
    pthread_cond_t* condition;  // Uslovna promenljiva za sinhronizaciju
    int* counter;       // Brojač za praćenje koliko niti je završilo fazu rotacije
} ThreadData;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex za sinhronizaciju rotacija redova
pthread_cond_t condition = PTHREAD_COND_INITIALIZER; // Uslovna promenljiva za sinhronizaciju

int rows_rotated = 0;           // Broj rotiranih redova
int total_threads_completed = 0; // Broj niti koje su završile kompletno izvršavanje

// Funkcija za rotaciju redova matrice
void rotate_rows(int* row, int m, int d) {
    int temp[m];
    // Niz temp se koristi za upisivanje reda i, tako sto ce vrednosti biti zarotirane za jedno mesto u desno
    // Primer: 
    // row[0] = [1,2,3,4]
    // temp = [4,1,2,3]
    // Zatim ce se trenutni red row[i] koji se rotira zameniti vrednostima iz temp-a
    for (int i = 0; i < m; i++) {
        temp[(i + d) % m] = row[i];
    }
    for (int i = 0; i < m; i++) {
        row[i] = temp[i];
    }
}

// Funkcija za rotaciju kolona matrice
/*
    Ova funkcija rotira kolone za jedno polje od dna ka vrhu
    Ako je kolona bila: 1 ona ce postati -> 2
                        2                   3   
                        3                   4
                        4                   1
*/          
void rotate_columns(int** matrix, int m, int n, int col) {
    int temp = matrix[0][col];
    for (int i = 0; i < m-1; i++) {
        matrix[i][col] = matrix[i + 1][col];
    }
    matrix[m-1][col] = temp;
}

// Funkcija koju će niti izvršavati
void* thread_function(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    // opisano u prethodnom zadatku
    int start_row = data->thread_id * (data->m / data->d);
    int end_row = (data->thread_id == data->d - 1) ? data->m : (data->thread_id + 1) * (data->m / data->d);

    // Rotacija redova
    for (int r = 0; r < data->s; r++) {
        for (int i = start_row; i < end_row; i++) {
            // svaka nit izvrsava rotaciju odgovarajuceg reda
            rotate_rows(data->matrix[i], data->m, 1);
        }

        /*
        Ova linija zaključava mutex koji se koristi za sinhronizaciju. Zaključavanjem mutexa osiguravamo da samo jedna nit može 
        istovremeno pristupiti delu koda koji je unutar ovog zaključanog dela. To osigurava da nijedna druga nit neće ući u ovaj 
        deo koda dok jedna nit ne završi svoje izvršavanje unutar zaključanog dela.
        */
        pthread_mutex_lock(data->mutex);

        /*
        Ovde se inkrementira brojač counter koji prati koliko niti je završilo rotaciju redova. 
        Svaka nit koja završi sa rotacijom redova povećava ovaj brojač.
        */
        (*(data->counter))++;
        
        /*
        Ovde se proverava da li je broj niti koje su završile rotaciju redova manji od ukupnog broja niti data->d koje su učestvovale 
        u rotaciji. Ako jeste, to znači da još uvek nisu sve niti završile rotaciju redova i trenutna nit čeka (blokira se) na 
        uslovnoj promenljivoj data->condition. Ovo osigurava da niti neće ići dalje dok sve niti ne završe rotaciju redova.
        */
        if (*(data->counter) < data->d) {
            // Ova funkcija čeka dok ne bude ispunjen određeni uslov na uslovnoj promenljivoj data->condition. 
            // U ovom slučaju, nit čeka dok ne budu završene sve niti rotacije redova. Kada se uslov ispuni, nit se odblokira i nastavlja izvršavanje.
            pthread_cond_wait(data->condition, data->mutex);
        } else {
            // Ako je broj niti koje su završile rotaciju redova jednak ukupnom broju niti data->d, to znači da su sve niti završile
            // rotaciju redova i treba odblokirati sve niti koje čekaju na uslovnoj promenljivoj data->condition.
            *(data->counter) = 0; // Brojač se resetuje na nulu kako bi mogao da se koristi za praćenje rotacije kolona nakon rotacije redova.

            // Ova funkcija obaveštava sve niti koje čekaju na uslovnoj promenljivoj data->condition da je uslov ispunjen. 
            // Sve ove niti će biti odblokirane i nastaviće izvršavanje.
            pthread_cond_broadcast(data->condition);
        }
        // mutex se odblokira kako bi druge niti mogle pristupiti ovom delu koda.
        pthread_mutex_unlock(data->mutex);

        for (int i = start_row; i < end_row; i++) {
            // svaka nit izvrsava rotaciju odgovarajuce kolone
            // i uzima vrednost iz [start_row, end_row) zato sto je matrica m x m dimenzija, pa je podela kolona i redova izmedju niti ista
            rotate_columns(data->matrix, data->m, data->m, i);
        }

        // Objasnjenje ovog dela je identicno kao za rotacije vrste (prethodni deo koda)
        pthread_mutex_lock(data->mutex);
        (*(data->counter))++;
        if (*(data->counter) < data->d) {
            pthread_cond_wait(data->condition, data->mutex);
        } else {
            *(data->counter) = 0;
            pthread_cond_broadcast(data->condition);
        }
        pthread_mutex_unlock(data->mutex);
    }

    pthread_exit(NULL);
}

int main() {
    int m; // Broj redova i kolona matrice
    int s, d; // Brojevi za rotaciju

    // Učitavanje brojeva s i d sa tastature
    printf("Unesite broj s: ");
    scanf("%d", &s);
    printf("Unesite broj d: ");
    scanf("%d", &d);

    // Učitavanje matrice iz fajla input.txt
    FILE* file = fopen("input_z2.txt", "r");
    fscanf(file, "%d", &m);

    int** matrix = (int**)malloc(m * sizeof(int*));
    for (int i = 0; i < m; i++) {
        matrix[i] = (int*)malloc(m * sizeof(int));
        for (int j = 0; j < m; j++) {
            fscanf(file, "%d", &matrix[i][j]);
        }
    }
    fclose(file);

    printf("Original Matrix:\n");
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < m; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }

    // Inicijalizacija niti, mutex-a i condition-a
    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];
    pthread_mutex_t mutex;
    pthread_cond_t condition;

    int counter = 0;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condition, NULL);

    for (int i = 0; i < d; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].d = d;
        thread_data[i].s = s;
        thread_data[i].m = m;
        thread_data[i].matrix = matrix;
        thread_data[i].mutex = &mutex;
        thread_data[i].condition = &condition;
        thread_data[i].counter = &counter;
        pthread_create(&threads[i], NULL, thread_function, (void*)&thread_data[i]);
    }

    for (int i = 0; i < d; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\nRotirana matrica:\n");
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < m; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }

    // Oslobađanje memorije
    for (int i = 0; i < m; i++) {
        free(matrix[i]);
    }
    free(matrix);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condition);

    return 0;
}
