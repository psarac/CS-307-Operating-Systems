#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/** 
 * Demo participation parts of this code is adapted from the H2O problem mentioned in the Little Book of Semaphores (p 143)
 * Written by Allen B. Downey
*/

//IMPLEMENTING SEMAPHORES LIKE IN LECTURES
typedef struct semaphore {
    int val;
    pthread_mutex_t lock;
    pthread_cond_t c;
} sem_t;

void sem_init (sem_t *s, int initVal) { //Initialization
    pthread_mutex_init(&s->lock, NULL);
    pthread_cond_init(&s->c, NULL);
    s->val = initVal;
}

void sem_wait(sem_t *s) { //Waiting
    //decrement the value of s by one
    //wait if the value of s is negative
    pthread_mutex_lock(&s->lock);
    s->val--;
    if (s->val < 0) 
        pthread_cond_wait(&s->c, &s->lock);
    pthread_mutex_unlock(&s->lock);
}

void sem_post(sem_t *s) { //Continuing 
    //increment the value of s by one
    //wake a sleeping thread if exists
    pthread_mutex_lock(&s->lock);
    s->val++;
    pthread_cond_signal(&s->c);
    pthread_mutex_unlock(&s->lock);
}

//PRIMITIVE DECLERATIONS HERE 

//DEMO PRIMITIVES
sem_t demo_mutex;
int demo_waiting_assistant = 0;
int demo_waiting_student = 0;
sem_t demo_assistant_queue;
sem_t demo_student_queue;
pthread_barrier_t demo_barrier;

//ENTERING PRIMITIVES
int enter_token = 0;
pthread_mutex_t enter_mutex;
pthread_cond_t enter_queue;


void* assistantFunc(void* arg) {

    pthread_mutex_lock(&enter_mutex);
        enter_token += 3;
        printf("Thread ID: %ld, Role:Assistant, I entered the classroom.\n", pthread_self());
        pthread_cond_signal(&enter_queue);
        pthread_cond_signal(&enter_queue);
        pthread_cond_signal(&enter_queue);
    pthread_mutex_unlock(&enter_mutex);

    sem_wait(&demo_mutex);
    demo_waiting_assistant++;
    if (demo_waiting_student >= 2) {
        sem_post(&demo_student_queue);
        sem_post(&demo_student_queue);
        demo_waiting_student -= 2;
        sem_post(&demo_assistant_queue);
        demo_waiting_assistant--;
    } else {
        sem_post(&demo_mutex);
    }

    sem_wait(&demo_assistant_queue);

    printf("Thread ID: %ld, Role:Assistant, I am now participating.\n", pthread_self());

    pthread_barrier_wait(&demo_barrier);
    sem_post(&demo_mutex);

    printf("Thread ID: %ld, Role:Assistant, demo is over.\n", pthread_self());

    printf("Thread ID: %ld, Role:Assistant, I left the classroom.\n", pthread_self());
}
 
void* studentFunc(void* arg) {

    pthread_mutex_lock(&enter_mutex);
        enter_token--;
        printf("Thread ID: %ld, Role:Student, I want to enter the classroom.\n", pthread_self());
        if (enter_token < 0) {
            pthread_cond_wait(&enter_queue, &enter_mutex);
        }
        printf("Thread ID: %ld, Role:Student, I entered the classroom.\n", pthread_self());
    pthread_mutex_unlock(&enter_mutex);


    sem_wait(&demo_mutex);
    demo_waiting_student++;
    if (demo_waiting_student >= 2 && demo_waiting_assistant >= 1) {
        sem_post(&demo_student_queue);
        sem_post(&demo_student_queue);
        demo_waiting_student -= 2;
        sem_post(&demo_assistant_queue);
        demo_waiting_assistant--;
    } else {
        sem_post(&demo_mutex);
    }

    sem_wait(&demo_student_queue);

    printf("Thread ID: %ld, Role:Student, I am now participating.\n", pthread_self());

    pthread_barrier_wait(&demo_barrier);

    printf("Thread ID: %ld, Role:Student, I left the classroom.\n", pthread_self());
}

int main (int argc, char *argv[]) {

    printf("My program complies with entering and grouping conditions, it has an extra restriction. There is only a single demo taking place at a time.\n");

    int assistantNum, studentNum;
    assistantNum = atoi(argv[1]);
    //printf("%d\n", assistantNum);
    studentNum = atoi(argv[2]);
    //printf("%d \n", studentNum);

    if (assistantNum <= 0 || 2*assistantNum != studentNum) {
        printf("The main terminates.\n");
        return 0;
    }

    //PRIMITIVE INITIALIZATIONS HERE

    //Demo initializations
    sem_init(&demo_mutex, 1);
    sem_init(&demo_assistant_queue, 0);
    sem_init(&demo_student_queue, 0);
    int ret = pthread_barrier_init(&demo_barrier, NULL, 3);
    if (ret) { printf("Fail to initialize barrier\n");} //not sure about this

    //Enter initializations
    pthread_mutex_init(&enter_mutex, NULL);
    pthread_cond_init(&enter_queue, NULL);

    //printf("Good to go\n");
    int totalNum = assistantNum + studentNum;

    //List to keep track of threads
    //pthread_t * threadList = (pthread_t*)malloc(totalNum * sizeof(pthread_t));
    pthread_t threadList[totalNum];

    //Creating the threads 
    int i;

    for (i = 0; i < assistantNum; i++) {
        pthread_create(&threadList[i], NULL, assistantFunc, NULL);
    }

    for (; i < totalNum; i++) {
        pthread_create(&threadList[i], NULL, studentFunc, NULL);
    }

    //Waiting for the threads
    for (int j = 0; j < totalNum; j++) {
        pthread_join(threadList[j], NULL);
    }

    pthread_barrier_destroy(&demo_barrier);

    printf("The main terminates.\n");

    return 0;
}