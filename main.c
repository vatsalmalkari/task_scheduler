#include "scheduler.h"
#include "app_tasks.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For sleep() in main to simulate other work
#include <pthread.h> // For pthread_create, pthread_join
#include <string.h> // For memset
#include <stdint.h> // For uint32_t type
int main() {
    printf("Starting main application.\n");

    // 1. Initialize the scheduler - MUST be called first!
    scheduler_init();

    printf("Scheduling tasks...\n");

    // Schedule a simple one-shot task: runs once after 1 second (1000ms)
    task_id_t task1_id = schedule_once(1000, my_task_function, "First task (1 second delay)");

    // Schedule a one-shot task with a dynamically allocated argument: runs once after 3 seconds (3000ms)
    int *val_for_task_once = malloc(sizeof(int));
    if (val_for_task_once == NULL) { perror("malloc failed"); return EXIT_FAILURE; }
    *val_for_task_once = 42;
    task_id_t task_once_id = schedule_once(3000, another_task, val_for_task_once);

    // Schedule a periodic task: starts after 0.5s (500ms), repeats every 1s (1000ms)
    task_id_t periodic_task_id1 = schedule_periodic(500, 1000, periodic_task_func, "Every 1s from 0.5s mark");

    // Schedule another periodic task: starts after 2s (2000ms), repeats every 2s (2000ms)
    task_id_t periodic_task_id2 = schedule_periodic(2000, 2000, periodic_task_func, "Every 2s from 2s mark");

    // Demonstrate external cancellation from the main thread
    task_id_t cancel_me_task_id = schedule_once(4000, my_task_function, "I will be cancelled!");
    printf("Scheduled task to cancel: ID %u\n", cancel_me_task_id);

    printf("Main application doing some other work for 1.5 seconds...\n");
    sleep(1);
    printf("Main application continuing...\n");
    sleep(1);

    // Attempt to cancel a task from the main thread before scheduler_run starts fully processing
    printf("Attempting to cancel task ID %u from main thread...\n", cancel_me_task_id);
    if (cancel_task(cancel_me_task_id) == 0) {
        printf("Cancellation successful for ID %u.\n", cancel_me_task_id);
    } else {
        printf("Cancellation failed for ID %u.\n", cancel_me_task_id);
    }

    // Try cancelling a non-existent task
    printf("Attempting to cancel non-existent task ID %u...\n", INVALID_TASK_ID + 999);
    cancel_task(INVALID_TASK_ID + 999);

    // Schedule a task to cancel periodic_task_id1 after a few runs (e.g., 4.5 seconds)
    task_id_t *id_to_cancel1 = malloc(sizeof(task_id_t));
    if (id_to_cancel1 == NULL) { perror("malloc failed"); return EXIT_FAILURE; }
    *id_to_cancel1 = periodic_task_id1;
    schedule_once(4500, cancellation_task, id_to_cancel1);

    // Schedule a task to cancel periodic_task_id2 after a few runs (e.g., 6.5 seconds)
    task_id_t *id_to_cancel2 = malloc(sizeof(task_id_t));
    if (id_to_cancel2 == NULL) { perror("malloc failed"); return EXIT_FAILURE; }
    *id_to_cancel2 = periodic_task_id2;
    schedule_once(6500, cancellation_task, id_to_cancel2);

    // Create a thread to schedule a new task while the scheduler is running
    printf("Creating a thread to schedule a new task...\n");
    pthread_t tid;
    if (pthread_create(&tid, NULL, thread_func, NULL) != 0) {
        perror("Failed to create thread");
        return EXIT_FAILURE;
    }

    // 3. Run the scheduler. This call blocks until all one-shot tasks have completed
    //    and all periodic tasks have been explicitly cancelled.
    printf("--- Scheduler is now running ---\n");
    scheduler_run();

    // 4. Join the background thread - good practice before shutdown
    pthread_join(tid, NULL);

    // 5. Shutdown the scheduler - frees all internal resources
    scheduler_shutdown();

    printf("--- Main application finished ---\n");
    return EXIT_SUCCESS;
}