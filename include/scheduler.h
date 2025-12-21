#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h> // For size_t
#include <stdint.h> // For uint64_t
#include <stdbool.h>

// Define a unique ID type for tasks
typedef uint32_t task_id_t;
#define INVALID_TASK_ID 0 // A value to indicate an invalid or failed task ID

// Define a function pointer type for tasks
typedef void (*task_func_t)(void *arg);

// Structure to represent a single scheduled task
typedef struct {
    task_id_t id;             // Unique ID for the task
    uint64_t execute_time_ms; // When to execute (absolute milliseconds from an epoch)
    task_func_t func;           // The function to call
    void *arg;           // Argument to pass to the function
    _Bool is_periodic;    // Is this a repeating task?
    uint64_t interval_ms;    // If periodic, how often does it repeat?
    _Bool arg_is_heap;
    _Bool cancelled;
} scheduled_task_t;

// Initialize the scheduler
void scheduler_init();

// Schedule a task to run once after a delay
// Returns the task_id_t on success, INVALID_TASK_ID on failure
task_id_t schedule_once(uint64_t delay_ms,task_func_t func,void *arg, bool arg_is_heap);
// Schedule a task to run periodically after an initial delay
// Returns the task_id_t on success, INVALID_TASK_ID on failure
task_id_t schedule_periodic(uint64_t initial_delay_ms, uint64_t interval_ms, task_func_t func, void *arg, bool arg_is_heap);

// Cancel a scheduled task by its ID
// Returns 0 on success, -1 if task not found
int cancel_task(task_id_t id);

// The main loop that runs the scheduler
void scheduler_run();

// Graceful shutdown
void scheduler_shutdown();

#endif // SCHEDULER_H