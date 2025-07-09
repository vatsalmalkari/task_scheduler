#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For sleep() in main to simulate other work
#include <pthread.h> // For pthread_create, pthread_join

// A simple task function - DOES NOT FREE ITS ARGUMENT
void my_task_function(void *arg) {
    printf("[Task Executed] -- My task function executed! Argument: %s\n", (char *)arg);
}

// Another task function - DOES FREE ITS ARGUMENT
void another_task(void *arg) {
    int *val = (int *)arg;
    printf("[Task Executed] -- Another task! Value: %d, attempting to free address %p\n", *val, (void*)val);
    free(val);
    printf("[Task Executed] -- Another task! Free successful for address %p\n", (void*)val);
}

// A periodic task function
void periodic_task_func(void *arg) {
    static int count = 0;
    printf("[Periodic Task] -- This task has run %d times. Argument: %s\n", ++count, (char*)arg);
}

// A task to cancel other tasks
void cancellation_task(void *arg) {
    task_id_t *task_id_to_cancel = (task_id_t *)arg;
    printf("[Cancellation Task] -- Attempting to cancel task ID %u from within a task!\n", *task_id_to_cancel);
    if (cancel_task(*task_id_to_cancel) == 0) {
        printf("[Cancellation Task] -- Successfully cancelled task ID %u.\n", *task_id_to_cancel);
    } else {
        printf("[Cancellation Task] -- Failed to cancel task ID %u.\n", *task_id_to_cancel);
    }
    free(task_id_to_cancel); // Free the malloc'd ID
}

void *thread_func(void *arg) {
    (void)arg; // Cast to void to suppress unused parameter warning
    sleep(3); // Wait for scheduler to start
    printf("\n[Thread]: Attempting to schedule a new task from a separate thread...\n");
    // This will now be thread-safe due to mutexes
    schedule_once(100, my_task_function, "Task from another thread!");
    printf("[Thread]: New task scheduled.\n");
    return NULL;
}


int main() {
    printf("Starting main application.\n");

    scheduler_init();

    printf("Scheduling tasks...\n");

    // One-shot tasks (from previous step)
    task_id_t task1_id = schedule_once(1000, my_task_function, "First task (1 second delay)"); // String literal
    int *val_for_task_once = malloc(sizeof(int));
    if (val_for_task_once == NULL) { perror("malloc failed"); return EXIT_FAILURE; }
    *val_for_task_once = 42;
    task_id_t task_once_id = schedule_once(3000, another_task, val_for_task_once); // Malloc'd int

    // New: Periodic tasks
    task_id_t periodic_task_id1 = schedule_periodic(500, 1000, periodic_task_func, "Every 1s from 0.5s mark");
    task_id_t periodic_task_id2 = schedule_periodic(2000, 2000, periodic_task_func, "Every 2s from 2s mark");

    // Demonstrate external cancellation
    task_id_t cancel_me_task_id = schedule_once(4000, my_task_function, "I will be cancelled!");
    printf("Scheduled task to cancel: ID %u\n", cancel_me_task_id);

    printf("Main application doing some other work for 1.5 seconds...\n");
    sleep(1);
    printf("Main application continuing...\n");
    sleep(1);

    // Cancel a task from main thread *before* scheduler_run starts fully processing
    printf("Attempting to cancel task ID %u from main thread...\n", cancel_me_task_id);
    if (cancel_task(cancel_me_task_id) == 0) {
        printf("Cancellation successful for ID %u.\n", cancel_me_task_id);
    } else {
        printf("Cancellation failed for ID %u.\n", cancel_me_task_id);
    }

    // Try cancelling a non-existent task
    printf("Attempting to cancel non-existent task ID %u...\n", INVALID_TASK_ID + 999);
    cancel_task(INVALID_TASK_ID + 999);

    // NEW: Schedule a task to cancel periodic_task_id1 after a few runs (e.g., 4.5 seconds)
    task_id_t *id_to_cancel1 = malloc(sizeof(task_id_t));
    if (id_to_cancel1 == NULL) { perror("malloc failed"); return EXIT_FAILURE; }
    *id_to_cancel1 = periodic_task_id1;
    schedule_once(4500, cancellation_task, id_to_cancel1); // Cancel periodic_task_id1

    // NEW: Schedule a task to cancel periodic_task_id2 after a few runs (e.g., 6.5 seconds)
    task_id_t *id_to_cancel2 = malloc(sizeof(task_id_t));
    if (id_to_cancel2 == NULL) { perror("malloc failed"); return EXIT_FAILURE; }
    *id_to_cancel2 = periodic_task_id2;
    schedule_once(6500, cancellation_task, id_to_cancel2); // Cancel periodic_task_id2

    // Create a thread to schedule a new task while the scheduler is running
    printf("Creating a thread to schedule a new task...\n");
    pthread_t tid;
    if (pthread_create(&tid, NULL, thread_func, NULL) != 0) {
        perror("Failed to create thread");
        return EXIT_FAILURE;
    }

    // Run the scheduler. It will now naturally terminate once all one-shot
    // tasks have run and all periodic tasks have been cancelled.
    scheduler_run();

    scheduler_shutdown();

    printf("Main application finished.\n");
    return EXIT_SUCCESS;
}