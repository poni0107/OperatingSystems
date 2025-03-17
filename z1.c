#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#define MAX_CHILDREN 10

int child_count = 0; // Broj trenutno aktivnih dece
int children_ids[MAX_CHILDREN]; // Niz za čuvanje ID-eva aktivnih dece

// Handler za signale
void signal_handler(int signum) { 
    time_t current_time;
    time(&current_time);
    
    printf("Signal %s received at %s", (signum == SIGUSR1) ? "SIGUSR1" : "SIGUSR2", ctime(&current_time));
}

// Selection sort algoritam
void selection_sort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int min_idx = i;
        for (int j = i + 1; j < n; j++) {
            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
        }
        int temp = arr[min_idx];
        arr[min_idx] = arr[i];
        arr[i] = temp;
    }
}

// Insertion sort algoritam
void insertion_sort(int arr[], int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// Funkcija koju izvršava proces deteta
void child_process(int child_id, char* input_filename) {
    FILE* input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        perror("Failed to open input file");
        exit(1);
    }

    int m;
    // Ucitavanje m - predstavlja ukupan broj vrednosti za sortiranje
    fscanf(input_file, "%d", &m);
    
    int* numbers = (int*)malloc(m * sizeof(int));
    // Ucitavanje vrednosti brojeva za sortiranje iz druge linije ulaznog fajla
    for (int i = 0; i < m; i++) {
        fscanf(input_file, "%d", &numbers[i]);
    }
    fclose(input_file);
    
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    // Biranje tipa sortiranja u zavisnost od id-a deteta
    if (child_id % 2 == 0) {
        insertion_sort(numbers, m);
    } else {
        selection_sort(numbers, m);
    }

    // Svako dete se uspavljuje na vremenski period od [1,7] sekundi
    int sleep_time = rand() % 7 + 1;
    sleep(sleep_time);

    gettimeofday(&end_time, NULL);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    FILE* output_file = fopen("temp_output.txt", "w");
    fprintf(output_file, "%d", m);

    // Upis sortiranih vrednosti u temp_output file
    for (int i = 0; i < m; i++) {
        fprintf(output_file, " %d", numbers[i]);
    }
    fclose(output_file);

    // Ispisivanje vremena izvršavanja deteta
    printf("Child %d execution time: %.6f seconds\n", child_id, elapsed_time);

    // Slanje signala roditelju na osnovu ID-a deteta
    kill(getppid(), (child_id % 2 == 0) ? SIGUSR2 : SIGUSR1);

    exit(0);
}

int main(int argc, char* argv[]) {
    struct timeval start_time, end_time;
     // Vreme početka izvršavanja glavnog procesa
    gettimeofday(&start_time, NULL); 
    
    // Provera ispravnosti komandne linije
    // Pozivanje izvrsavanja izvrsnog fajla preko komandne linije zahteva pozivanje izvrsnog fajla i broj n -> broj dece
    if (argc != 2) {
        printf("Usage: %s n\n", argv[0]);
        return 1;
    }

    // Učitavanje broja dece iz komandne linije
    int n = atoi(argv[1]);
    if (n <= 0 || n > MAX_CHILDREN) {
        printf("Invalid number of children\n");
        return 1;
    }

   // Inicijalizacija generatora slučajnih brojeva
    srand(time(NULL));
    // Postavljanje handlera za signale
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

    // Kreiranje niza za čuvanje ID-eva dece
    for (int i = 0; i < n; i++) {
        pid_t child_pid = fork();
        if (child_pid == 0) {
            child_process(i + 1, "input.txt"); // Pokretanje procesa deteta
        } else if (child_pid > 0) {
            children_ids[child_count++] = child_pid; // Čuvanje ID-a deteta
        } else {
            perror("Failed to fork");
            return 1;
        }
    }

    // Čekanje da se završe sva deca
    while (child_count > 0) {
        int status;
        pid_t finished_child = wait(&status);
        if (finished_child > 0) {
            for (int i = 0; i < child_count; i++) {
                if (children_ids[i] == finished_child) {
                    children_ids[i] = children_ids[child_count - 1];
                    child_count--;
                    break;
                }
            }
        }
    }

    // Otvaranje izlaznog fajla za pisanje rezultata
    FILE* output_file = fopen("output.txt", "w");
    for (int i = 0; i < n; i++) {
        char signal_name[10];
        if (i % 2 == 0) {
            strcpy(signal_name, "SIGUSR2");
        } else {
            strcpy(signal_name, "SIGUSR1");
        }

        // Čitanje privremenog izlaznog fajla
        FILE* temp_output = fopen("temp_output.txt", "r");
        int m;
        fscanf(temp_output, "%d", &m);
        int* sorted_numbers = (int*)malloc(m * sizeof(int));
        for (int j = 0; j < m; j++) {
            fscanf(temp_output, "%d", &sorted_numbers[j]);
        }
        fclose(temp_output);

        // Izračunavanje proteklog vremena
        gettimeofday(&end_time, NULL); 
        double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0; // Calculate elapsed time

         // Pisanje rezultata u izlazni fajl
        fprintf(output_file, "Child %d execution time: %d, sorted numbers:", i + 1, (int)elapsed_time);
        for (int j = 0; j < m; j++) {
            fprintf(output_file, " %d", sorted_numbers[j]);
        }
        time_t current_time;
        time(&current_time);
        fprintf(output_file, " %s at %s\n", signal_name, ctime(&current_time));
    }
    
    fclose(output_file);    // Zatvaranje izlaznog fajla

    return 0;
}
