// In src/app_tasks.c (and declared in include/app_tasks.h)
#include "app_tasks.h"   // Include its own header for declarations
#include <stdio.h>       // For printf
#include <stdlib.h>      // For malloc/free
#include <unistd.h>      // For sleep() in thread_func
#include "scheduler.h"   // For schedule_once, cancel_task, task_id_t, INVALID_TASK_ID

// A simple one-shot task
void my_task_function(void *arg) {
    printf("[Task Executed] -- My task function executed! Argument: %s\n", (char *)arg);
}

// A one-shot task that handles freeing its dynamically allocated argument
void another_task(void *arg) {
    int *val = (int *)arg;
    printf("[Task Executed] -- Another task! Value: %d, attempting to free address %p\n", *val, (void*)val);
    free(val);
    printf("[Task Executed] -- Another task! Free successful for address %p\n", (void*)val);
}

// A task that runs periodically
void periodic_task_func(void *arg) {
    static int count = 0; // Static to keep count across calls
    printf("[Periodic Task] -- This task has run %d times. Argument: %s\n", ++count, (char*)arg);
}

// A task specifically designed to cancel another task
void cancellation_task(void *arg) {
    task_id_t *task_id_to_cancel = (task_id_t *)arg;
    printf("[Cancellation Task] -- Attempting to cancel task ID %u from within a task!\n", *task_id_to_cancel);
    if (cancel_task(*task_id_to_cancel) == 0) {
        printf("[Cancellation Task] -- Successfully cancelled task ID %u.\n", *task_id_to_cancel);
    } else {
        printf("[Cancellation Task] -- Failed to cancel task ID %u.\n", *task_id_to_cancel);
    }
    free(task_id_to_cancel); // Free the malloc'd ID that was passed to this task
}

// A function designed to be run in a separate thread to demonstrate concurrent scheduling
void *thread_func(void *arg) {
    (void)arg; // Cast to void to suppress unused parameter warning
    sleep(3); // Wait for main scheduler loop to start
    printf("\n[Thread]: Attempting to schedule a new task from a separate thread...\n");
    // This call is thread-safe!
    schedule_once(100, my_task_function, "Task from another thread!");
    printf("[Thread]: New task scheduled.\n");
    return NULL;
}