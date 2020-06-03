#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 3

int inputGamesNumber();    
int threadsHaveChosenRPS();
void *calculateScore();
void *printResultForIteration(int iteration);
int threadsHavePreparedForNewGame();
void *playRPS(void *id);

typedef struct {
    int gamesNum;
    /* 0 - not selected (default), 1 - rock, 2 - scissors, 3 - paper */
    int *thread_choice; 
    int *thread_score;

} GameData;

GameData gameData;
pthread_mutex_t thread_choice_mutex;
pthread_cond_t thread_choice_cv;


int main() {
    int i;
    long thread1_id=1, thread2_id=2, thread3_id=3;
    pthread_t threads[3];    
    pthread_attr_t attr;

    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&thread_choice_mutex, NULL);
    pthread_cond_init (&thread_choice_cv, NULL);

    int gamesNum = inputGamesNumber();    
    gameData.gamesNum = gamesNum;

    int *thread_choice;
    int *thread_score;

    /* Call calloc to allocate that appropriate number of bytes for the arrays and initialize them to 0 */
    thread_choice = (int *) calloc(NUM_THREADS, sizeof(int));       // allocate 3 ints
    thread_score = (int *) calloc(NUM_THREADS, sizeof(int));        // allocate 3 ints

    gameData.thread_choice = thread_choice;
    gameData.thread_score = thread_score;


    /* For portability, explicitly create threads in a joinable state */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&threads[0], &attr, playRPS, (void *)thread1_id);
    pthread_create(&threads[1], &attr, playRPS, (void *)thread2_id);
    pthread_create(&threads[2], &attr, playRPS, (void *)thread3_id);


    /* Wait for all threads to complete */
    for (i=0; i<NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    printf ("Main(): Waited on %d  threads. Done.\n", NUM_THREADS);

    printf("Final Score - Thread 1: %d, Thread 2: %d, Thread 3: %d\n", 
        gameData.thread_score[0], gameData.thread_score[1], gameData.thread_score[2]);

    /* Clean up and exit */
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&thread_choice_mutex);
    pthread_cond_destroy(&thread_choice_cv);
    pthread_exit(NULL);    

    return 0;
}      

int inputGamesNumber() {
    int gamesNum = 0;

    do {
        printf("Enter how many Rock Paper Scissors should be played (Min 1 and Max 100): ");
        scanf("%d", &gamesNum);
        if (gamesNum < 1 || gamesNum > 100) {
            printf("[ERROR] You did not enter a valid number\n");
            
            // flushing the input buffer in case scanf consumed an invalid char in the buffer (e.g. "ass" instead of number)
            int c; 
            while((c = getchar()) != '\n' && c != EOF);
        } 
    } while (gamesNum < 1 || gamesNum > 100);
    
    return gamesNum;
}

void *playRPS(void *id) {
    long my_id = (long)id;
    int i;

    for(i = 0; i < gameData.gamesNum; i++) {
        pthread_mutex_lock(&thread_choice_mutex);
        gameData.thread_choice[my_id-1] = rand() % 3 + 1;
        printf("Thread %ld has selected %d\n", my_id, gameData.thread_choice[my_id-1]);
        

        if(threadsHaveChosenRPS() == 0) {
            /* We will make this thread wait until the other threads select RPS */
            printf("Thread %ld is waiting for the other threads to choose RPS.\n", my_id);
            pthread_cond_wait(&thread_choice_cv, &thread_choice_mutex);
            printf("Thread %ld has stopped waiting for the other threads to choose RPS.\n", my_id);
        } else {
            /* All threads have selected RPS, so we are sending the signal to unblock the waiting and continue with the game.*/
            pthread_cond_broadcast(&thread_choice_cv);
            printf("Thread %ld is broadcasting that all threads have chosen RPS.\n", my_id);

            calculateScore(); // We only need to calculate the score once, so we'll do it from the last thread that selected RPS
            printResultForIteration(i); // We only need to print the game score once, so we'll do it from the last thread that selected RPS
        }
        
        gameData.thread_choice[my_id-1] = 0;
        printf("Thread %ld has reset RPS choice to %d\n", my_id, gameData.thread_choice[my_id-1]);

        if(threadsHavePreparedForNewGame() == 0) {
            /* We will make this thread wait until the other threads prepare for new game. */
            printf("Thread %ld is waiting for the other threads to reset RPS choice.\n", my_id);
            pthread_cond_wait(&thread_choice_cv, &thread_choice_mutex);
            printf("Thread %ld has stopped waiting for the other threads to reset RPS choice.\n", my_id);
        } else {
            /* All threads have prepared for new game, so we are sending the signal to unblock the waiting and continue with the game.*/            
            pthread_cond_broadcast(&thread_choice_cv);
            printf("Thread %ld is broadcasting that all threads have reset RPS choice.\n", my_id);
        }
        pthread_mutex_unlock(&thread_choice_mutex);
    }    
}

/* Return 1 if all threads have chosen Rock, Paper or Scissors and 0 if not all of them have */
int threadsHaveChosenRPS() {   
    int i, have_chosen_flag = 1;
    for(i = 0; i < NUM_THREADS; i++) {
        if (gameData.thread_choice[i] == 0) {
            printf("  Thread %d has not chosen RPS.\n", i + 1);
            have_chosen_flag = 0;
            break;
        }
        printf("  Thread %d has chosen RPS.\n", i + 1);
    }

    return have_chosen_flag;
}

void *calculateScore() {    
    int thread1_choice = gameData.thread_choice[0];
    int thread2_choice = gameData.thread_choice[1];
    int thread3_choice = gameData.thread_choice[2];    
   
    if (thread1_choice == 1) {
        if (thread2_choice == thread3_choice == 2) {
            gameData.thread_score[0] += 2;
        } else if (thread2_choice == 1 && thread3_choice == 2) {
            gameData.thread_score[0] += 1;
            gameData.thread_score[1] += 1;
        } else if (thread2_choice == 2 && thread3_choice == 1) {
            gameData.thread_score[0] += 1;
            gameData.thread_score[2] += 1;
        } else if (thread2_choice == 3 && thread3_choice == 3) {
            gameData.thread_score[1] += 1;
            gameData.thread_score[2] += 1;
        } else if (thread2_choice == 3 && thread3_choice == 1) {
            gameData.thread_score[1] += 2;
        } else if (thread2_choice == 1 && thread3_choice == 3) {
            gameData.thread_score[2] += 2;
        } 
    } else if (thread1_choice == 2) {
        if (thread2_choice == thread3_choice == 3) {
            gameData.thread_score[0] += 2;
        } else if (thread2_choice == 2 && thread3_choice == 3) {
            gameData.thread_score[0] += 1;
            gameData.thread_score[1] += 1;
        } else if (thread2_choice == 3 && thread3_choice == 2) {
            gameData.thread_score[0] += 1;
            gameData.thread_score[2] += 1;
        } else if (thread2_choice == 1 && thread3_choice == 1) {
            gameData.thread_score[1] += 1;
            gameData.thread_score[2] += 1;
        } else if (thread2_choice == 1 && thread3_choice == 2) {
            gameData.thread_score[1] += 2;
        } else if (thread2_choice == 2 && thread3_choice == 1) {
            gameData.thread_score[2] += 2;
        } 
    } else if (thread1_choice == 3) {
        if (thread2_choice == thread3_choice == 1) {
            gameData.thread_score[0] += 2;
        } else if (thread2_choice == 3 && thread3_choice == 1) {
            gameData.thread_score[0] += 1;
            gameData.thread_score[1] += 1;
        } else if (thread2_choice == 1 && thread3_choice == 3) {
            gameData.thread_score[0] += 1;
            gameData.thread_score[2] += 1;
        } else if (thread2_choice == 2 && thread3_choice == 2) {
            gameData.thread_score[1] += 1;
            gameData.thread_score[2] += 1;
        } else if (thread2_choice == 2 && thread3_choice == 3) {
            gameData.thread_score[1] += 2;
        } else if (thread2_choice == 3 && thread3_choice == 2) {
            gameData.thread_score[2] += 2;
        } 
    }
    
}

void *printResultForIteration(int iteration) {
    printf("Game %d - Thread 1: %d, Thread 2: %d, Thread 3: %d\n", 
        iteration + 1, gameData.thread_score[0], gameData.thread_score[1], gameData.thread_score[2]);
}

/* Return 1 if all threads have reset their respective thread choice and 0 if not all of them have */
int threadsHavePreparedForNewGame() {
    int i, have_prepared_flag = 1;
    for(i = 0; i < NUM_THREADS; i++) {
        if (gameData.thread_choice[i] != 0) {
            printf("  Thread %d has not prepared for new game.\n", i + 1);
            have_prepared_flag = 0;
            break;
        }
        printf("  Thread %d has prepared for new game.\n", i + 1);
    }

    return have_prepared_flag;
}