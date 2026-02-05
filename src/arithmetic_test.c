#include "scheduler.h"
#include "arith_tasks.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define NUM_TASKS 2000 
#define STRESS_DELAY_RANGE 1000 

static int executed = 0;
static pthread_mutex_t exec_lock = PTHREAD_MUTEX_INITIALIZER;

// WORKER FUNCTION 
void arithmetic_task(void *arg) {
    arithmetic_task_t *task = (arithmetic_task_t*)arg;
    execute_task(task);

    pthread_mutex_lock(&exec_lock);
    executed++;
    pthread_mutex_unlock(&exec_lock);
}

//  MAIN TEST HARNESS 
int arithmetic_test_main() {
    executed = 0; 
    
    fprintf(stderr, "\n[INIT] Starting Stress Test with %d Tasks \n", NUM_TASKS);
    
    scheduler_init();

    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, (void*(*)(void*))scheduler_run, NULL);

    time_t start_wall = time(NULL); 

    for (int i = 0; i < NUM_TASKS; i++) {
        arithmetic_task_t *task = make_random_task();
        uint64_t delay = rand() % STRESS_DELAY_RANGE;

        if (schedule_once(delay, arithmetic_task, task, true) == INVALID_TASK_ID) {
            printf("  [!] Schedule Overflow at task %d\n", i);
        }
    }
    fprintf(stderr, "[INFO] Injection complete. Monitoring execution \n");

    int last_reported = -1;
    while (1) {
        pthread_mutex_lock(&exec_lock);
        int current_done = executed;
        pthread_mutex_unlock(&exec_lock);

        // Update progress every 500 tasks 
        if (current_done / 500 > last_reported || current_done == NUM_TASKS) {
            fprintf(stderr, "  -> Progress: %d / %d tasks completed\n", current_done, NUM_TASKS);
            last_reported = current_done / 500;
        }

        if (current_done >= NUM_TASKS) break;
        usleep(100000); 
    }

    //  SHUTDOWN 
    
    scheduler_shutdown(); 

    pthread_join(scheduler_thread, NULL);

    // FINAL METRICS REPORTING
    time_t end_wall = time(NULL);
    
    printf("\n FINAL METRICS \n");
    printf("Total Runtime:   %ld seconds\n", end_wall - start_wall);
    printf("Tasks Processed: %d\n", executed);
    
    print_final_metrics(); 
    
    printf("STRESS TEST COMPLETE\n");
    fflush(stdout); 

    return 0;
}