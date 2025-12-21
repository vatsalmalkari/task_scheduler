#include "scheduler.h"
#include "arith_tasks.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define NUM_TASKS 1000
// Tracks executed tasks
static int executed = 0;
static pthread_mutex_t exec_lock = PTHREAD_MUTEX_INITIALIZER;

// Wrapper for scheduler
void arithmetic_task(void *arg) {
    arithmetic_task_t *task = (arithmetic_task_t*)arg;
    execute_task(task);

    pthread_mutex_lock(&exec_lock);
    executed++;
    if (executed % 50 == 0) {
        printf(" EXEC %d/%d tasks executed \n", executed, NUM_TASKS);
    }
    pthread_mutex_unlock(&exec_lock);
}

int main() {
    srand(time(NULL));

    printf("\n TEST Initializing scheduler \n");
    scheduler_init();

    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL,
                   (void*(*)(void*))scheduler_run, NULL);

    printf(" TEST Scheduling %d arithmetic tasks \n", NUM_TASKS);

    for (int i = 0; i < NUM_TASKS; i++) {
        arithmetic_task_t *task = make_random_task();
        int delay_ms = rand() % 800 + 100;  // 100–899 ms

        schedule_once(delay_ms, arithmetic_task, task, true);

        if (i % 100 == 0) {
            printf(" SCHED Scheduled %d/%d tasks...\n", i, NUM_TASKS);
        }
    }

    printf("TEST All tasks scheduled. Waiting for execution \n");

    while (1) {
        pthread_mutex_lock(&exec_lock);
        int done = executed;
        pthread_mutex_unlock(&exec_lock);

        if (done >= NUM_TASKS)
            break;

        usleep(10000); // 10 ms
    }

    pthread_join(scheduler_thread, NULL);
    scheduler_shutdown();

    printf("\n TEST Arithmetic stress test completed successfully \n");
    printf("SUMMARY %d/%d tasks executed \n", executed, NUM_TASKS);

    return 0;
}
