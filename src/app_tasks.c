#include "app_tasks.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// One-shot task
void my_task_function(void *arg) {
    printf("Task executed Argument: %s\n", (char *)arg);
}

// Periodic task
void periodic_task_func(void *arg) {
    static int count = 0;
    printf("Periodic Task Run %d times. Argument: %s\n", ++count, (char*)arg);
}

// Cancellation task
void cancellation_task(void *arg) {
    task_id_t *task_id_to_cancel = (task_id_t *)arg;
    if (cancel_task(*task_id_to_cancel) == 0)
        printf("Cancellation Task successfully cancelled task ID %u.\n", *task_id_to_cancel);
    else
        printf("Cancellation Task failed to cancel task ID %u.\n", *task_id_to_cancel);
    // DO NOT free(arg); scheduler handles heap arguments
}

// Thread demo task
void *thread_func(void *arg) {
    (void)arg;
    sleep(1);
    schedule_once(100, my_task_function, "Task from thread", false);
    return NULL;
}
