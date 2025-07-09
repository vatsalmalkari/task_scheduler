#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>      // For clock_gettime and CLOCK_MONOTONIC
#include <unistd.h>    // For usleep/nanosleep - will be replaced by cond_wait
#include <string.h>    // For memset, memcpy
#include <stdbool.h>   // For _Bool type
#include <pthread.h>   // For mutexes and condition variables

// --- Internal Data Structures for the Min-Heap ---

typedef struct {
    scheduled_task_t **tasks; // Array of pointers to tasks
    size_t capacity;          // Max number of tasks the array can hold
    size_t size;              // Current number of tasks in the heap
} task_heap_t;

static task_heap_t s_task_heap;      // Our global heap instance
static task_id_t s_next_task_id = 1; // Counter for unique task IDs

#define INITIAL_HEAP_CAPACITY 10

// --- Hash Table for Task ID to Heap Index Mapping ---
#define HASH_TABLE_CAPACITY 100
typedef struct {
    task_id_t id;
    size_t heap_idx;
    bool occupied;
} hash_entry_t;

static hash_entry_t s_task_id_map[HASH_TABLE_CAPACITY];
static size_t s_map_size = 0;

// --- Mutex and Condition Variable for Thread Safety ---
static pthread_mutex_t s_scheduler_mutex;
static pthread_cond_t s_scheduler_cond; // NEW: Condition variable


// --- Forward Declarations for Internal Helper Functions ---
static uint64_t get_current_time_ms();
static int resize_heap_if_needed();
static void swap(size_t i, size_t j);
static void heapify_up(size_t index);
static void heapify_down(size_t index);
static void map_insert(task_id_t id, size_t heap_idx);
static bool map_get(task_id_t id, size_t *heap_idx);
static void map_remove(task_id_t id);
static task_id_t create_and_schedule_task(uint64_t delay_ms, task_func_t func, void *arg, bool is_periodic, uint64_t interval_ms);


// --- Helper Functions (Internal) ---

static uint64_t get_current_time_ms() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static void swap(size_t i, size_t j) {
    if (i == j) return;
    scheduled_task_t *temp = s_task_heap.tasks[i];
    s_task_heap.tasks[i] = s_task_heap.tasks[j];
    s_task_heap.tasks[j] = temp;

    map_insert(s_task_heap.tasks[i]->id, i);
    map_insert(s_task_heap.tasks[j]->id, j);
}

static void heapify_up(size_t index) {
    size_t parent_index;
    while (index > 0) {
        parent_index = (index - 1) / 2;
        if (s_task_heap.tasks[index]->execute_time_ms < s_task_heap.tasks[parent_index]->execute_time_ms) {
            swap(index, parent_index);
            index = parent_index;
        } else {
            break;
        }
    }
}

static void heapify_down(size_t index) {
    size_t left_child, right_child, smallest_child;
    while (1) {
        left_child = 2 * index + 1;
        right_child = 2 * index + 2;
        smallest_child = index;

        if (left_child < s_task_heap.size &&
            s_task_heap.tasks[left_child]->execute_time_ms < s_task_heap.tasks[smallest_child]->execute_time_ms) {
            smallest_child = left_child;
        }

        if (right_child < s_task_heap.size &&
            s_task_heap.tasks[right_child]->execute_time_ms < s_task_heap.tasks[smallest_child]->execute_time_ms) {
            smallest_child = right_child;
        }

        if (smallest_child != index) {
            swap(index, smallest_child);
            index = smallest_child;
        } else {
            break;
        }
    }
}

static int resize_heap_if_needed() {
    if (s_task_heap.size == s_task_heap.capacity) {
        size_t new_capacity = s_task_heap.capacity == 0 ? INITIAL_HEAP_CAPACITY : s_task_heap.capacity * 2;
        scheduled_task_t **new_tasks = realloc(s_task_heap.tasks, new_capacity * sizeof(scheduled_task_t *));
        if (!new_tasks) {
            perror("Failed to resize heap");
            return -1;
        }
        s_task_heap.tasks = new_tasks;
        s_task_heap.capacity = new_capacity;
        printf("Heap resized to capacity: %zu\n", s_task_heap.capacity);
    }
    return 0;
}

// --- Hash Table Helper Functions (no changes needed) ---
static size_t hash_func(task_id_t id) { return id % HASH_TABLE_CAPACITY; }
static void map_insert(task_id_t id, size_t heap_idx) {
    size_t hash = hash_func(id); size_t original_hash = hash;
    do {
        if (!s_task_id_map[hash].occupied || s_task_id_map[hash].id == id) {
            s_task_id_map[hash].id = id; s_task_id_map[hash].heap_idx = heap_idx; s_task_id_map[hash].occupied = true;
            if (s_task_id_map[hash].id != id) s_map_size++; return;
        }
        hash = (hash + 1) % HASH_TABLE_CAPACITY;
    } while (hash != original_hash);
    fprintf(stderr, "Error: Hash table full, cannot insert task ID %u\n", id);
}
static bool map_get(task_id_t id, size_t *heap_idx) {
    size_t hash = hash_func(id); size_t original_hash = hash;
    do {
        if (s_task_id_map[hash].occupied && s_task_id_map[hash].id == id) {
            *heap_idx = s_task_id_map[hash].heap_idx; return true;
        }
        if (!s_task_id_map[hash].occupied && s_task_id_map[hash].id == INVALID_TASK_ID) { return false; }
        hash = (hash + 1) % HASH_TABLE_CAPACITY;
    } while (hash != original_hash); return false;
}
static void map_remove(task_id_t id) {
    size_t hash = hash_func(id); size_t original_hash = hash;
    do {
        if (s_task_id_map[hash].occupied && s_task_id_map[hash].id == id) {
            s_task_id_map[hash].occupied = false; s_task_id_map[hash].id = INVALID_TASK_ID; s_map_size--; return;
        }
        if (!s_task_id_map[hash].occupied && s_task_id_map[hash].id == INVALID_TASK_ID) { return; }
        hash = (hash + 1) % HASH_TABLE_CAPACITY;
    } while (hash != original_hash);
}


// Helper to create and insert a task into the heap
static task_id_t create_and_schedule_task(uint64_t delay_ms, task_func_t func, void *arg, bool is_periodic, uint64_t interval_ms) {
    if (!func) {
        fprintf(stderr, "Error: Cannot schedule NULL function.\n");
        return INVALID_TASK_ID;
    }

    pthread_mutex_lock(&s_scheduler_mutex); // Acquire lock

    if (resize_heap_if_needed() != 0) {
        pthread_mutex_unlock(&s_scheduler_mutex);
        return INVALID_TASK_ID;
    }

    scheduled_task_t *new_task = (scheduled_task_t *)malloc(sizeof(scheduled_task_t));
    if (!new_task) {
        perror("Failed to allocate memory for new task struct");
        pthread_mutex_unlock(&s_scheduler_mutex);
        return INVALID_TASK_ID;
    }
    memset(new_task, 0, sizeof(scheduled_task_t));

    new_task->id = s_next_task_id++;
    new_task->execute_time_ms = get_current_time_ms() + delay_ms;
    new_task->func = func;
    new_task->arg = arg;
    new_task->is_periodic = is_periodic;
    new_task->interval_ms = interval_ms;

    s_task_heap.tasks[s_task_heap.size] = new_task;
    s_task_heap.size++;
    map_insert(new_task->id, s_task_heap.size - 1);
    heapify_up(s_task_heap.size - 1);

    printf("Task ID %u scheduled for execution at %llu ms (Current: %llu, Delay: %llu ms). %s\n",
           new_task->id,
           (unsigned long long)new_task->execute_time_ms,
           (unsigned long long)get_current_time_ms(),
           (unsigned long long)delay_ms,
           is_periodic ? "(Periodic)" : "(Once)");

    // NEW: Signal the scheduler thread that a new task might be due earlier
    pthread_cond_signal(&s_scheduler_cond);

    pthread_mutex_unlock(&s_scheduler_mutex); // Release lock

    return new_task->id;
}


// --- Scheduler API Implementations ---

void scheduler_init() {
    printf("Scheduler initialized.\n");

    // Initialize mutex and condition variable
    if (pthread_mutex_init(&s_scheduler_mutex, NULL) != 0) {
        perror("Failed to initialize scheduler mutex");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&s_scheduler_cond, NULL) != 0) { // NEW: Init cond var
        perror("Failed to initialize scheduler condition variable");
        pthread_mutex_destroy(&s_scheduler_mutex); // Clean up mutex if cond fails
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&s_scheduler_mutex); // Acquire lock

    s_task_heap.tasks = (scheduled_task_t **)malloc(INITIAL_HEAP_CAPACITY * sizeof(scheduled_task_t *));
    if (!s_task_heap.tasks) {
        perror("Failed to allocate initial heap memory");
        pthread_mutex_unlock(&s_scheduler_mutex);
        exit(EXIT_FAILURE);
    }
    s_task_heap.capacity = INITIAL_HEAP_CAPACITY;
    s_task_heap.size = 0;
    s_next_task_id = 1;
    memset(s_task_heap.tasks, 0, INITIAL_HEAP_CAPACITY * sizeof(scheduled_task_t *));

    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        s_task_id_map[i].occupied = false;
        s_task_id_map[i].id = INVALID_TASK_ID;
    }
    s_map_size = 0;

    pthread_mutex_unlock(&s_scheduler_mutex); // Release lock
}


task_id_t schedule_once(uint64_t delay_ms, task_func_t func, void *arg) {
    return create_and_schedule_task(delay_ms, func, arg, false, 0);
}

task_id_t schedule_periodic(uint64_t initial_delay_ms, uint64_t interval_ms, task_func_t func, void *arg) {
    if (interval_ms == 0) {
        fprintf(stderr, "Warning: Periodic task with 0 interval_ms. Consider using schedule_once.\n");
        return schedule_once(initial_delay_ms, func, arg);
    }
    return create_and_schedule_task(initial_delay_ms, func, arg, true, interval_ms);
}


int cancel_task(task_id_t id) {
    if (id == INVALID_TASK_ID) {
        fprintf(stderr, "Error: Attempted to cancel an INVALID_TASK_ID.\n");
        return -1;
    }

    pthread_mutex_lock(&s_scheduler_mutex); // Acquire lock

    size_t heap_idx;
    if (!map_get(id, &heap_idx)) {
        printf("Attempted to cancel Task ID %u, but it was not found.\n", id);
        pthread_mutex_unlock(&s_scheduler_mutex);
        return -1;
    }

    printf("Canceling Task ID %u at heap index %zu.\n", id, heap_idx);

    scheduled_task_t *task_to_cancel = s_task_heap.tasks[heap_idx];

    s_task_heap.size--;
    map_remove(task_to_cancel->id);

    if (s_task_heap.size > 0 && heap_idx != s_task_heap.size) {
        s_task_heap.tasks[heap_idx] = s_task_heap.tasks[s_task_heap.size];
        map_insert(s_task_heap.tasks[heap_idx]->id, heap_idx);
        heapify_down(heap_idx);
        heapify_up(heap_idx);
    }

    free(task_to_cancel);

    printf("Task ID %u successfully canceled and freed.\n", id);

    // NEW: Signal the scheduler thread in case cancellation affects the next due task
    pthread_cond_signal(&s_scheduler_cond);

    pthread_mutex_unlock(&s_scheduler_mutex); // Release lock
    return 0;
}


void scheduler_run() {
    printf("Scheduler is now running...\n");
    uint64_t current_time;

    while (1) {
        pthread_mutex_lock(&s_scheduler_mutex); // Acquire lock at start of loop

        current_time = get_current_time_ms();

        // Process all currently due tasks
        while (s_task_heap.size > 0 && s_task_heap.tasks[0]->execute_time_ms <= current_time) {
            scheduled_task_t *task_to_execute = s_task_heap.tasks[0];

            // Remove the root (min element) from the heap and map
            s_task_heap.size--;
            map_remove(task_to_execute->id);

            if (s_task_heap.size > 0) {
                s_task_heap.tasks[0] = s_task_heap.tasks[s_task_heap.size];
                map_insert(s_task_heap.tasks[0]->id, 0);
                heapify_down(0);
            }

            // --- IMPORTANT: Release lock before executing task ---
            // This allows other threads to schedule/cancel tasks while this task runs.
            pthread_mutex_unlock(&s_scheduler_mutex);

            printf("Executing Task ID %u (scheduled for %llu ms, current %llu ms)...\n",
                   task_to_execute->id,
                   (unsigned long long)task_to_execute->execute_time_ms,
                   (unsigned long long)current_time);

            if (task_to_execute->func) {
                task_to_execute->func(task_to_execute->arg);
            } else {
                fprintf(stderr, "Warning: Task ID %u had a NULL function pointer!\n", task_to_execute->id);
            }

            // --- Re-acquire lock to continue scheduler logic (reschedule/free) ---
            pthread_mutex_lock(&s_scheduler_mutex);

            if (task_to_execute->is_periodic) {
                printf("Task ID %u is periodic. Re-scheduling for %llu ms from now.\n",
                       task_to_execute->id, (unsigned long long)task_to_execute->interval_ms);
                task_to_execute->execute_time_ms = current_time + task_to_execute->interval_ms;

                if (resize_heap_if_needed() != 0) {
                    fprintf(stderr, "Error: Failed to re-schedule periodic task %u due to heap resize failure.\n", task_to_execute->id);
                    free(task_to_execute);
                } else {
                    s_task_heap.tasks[s_task_heap.size] = task_to_execute;
                    s_task_heap.size++;
                    map_insert(task_to_execute->id, s_task_heap.size - 1);
                    heapify_up(s_task_heap.size - 1);
                }
            } else {
                free(task_to_execute);
            }
            current_time = get_current_time_ms(); // Update current time after execution/reschedule

            // Loop back to check if more tasks are due WITHOUT releasing mutex
            // The inner while loop processes all due tasks under one mutex acquisition.
        } // End of inner while loop (processing all currently due tasks)

        // Determine how long to sleep / wait
        if (s_task_heap.size > 0) {
            uint64_t next_task_time = s_task_heap.tasks[0]->execute_time_ms;
            long long sleep_duration_ms = (long long)next_task_time - (long long)current_time;

            if (sleep_duration_ms > 0) {
                struct timespec ts_wait;
                // Calculate absolute time for pthread_cond_timedwait
                struct timespec current_ts;
                clock_gettime(CLOCK_MONOTONIC, &current_ts); // Get current time for absolute timeout

                ts_wait.tv_sec = current_ts.tv_sec + (sleep_duration_ms / 1000);
                ts_wait.tv_nsec = current_ts.tv_nsec + (sleep_duration_ms % 1000) * 1000000;

                // Handle nanosecond overflow
                if (ts_wait.tv_nsec >= 1000000000) {
                    ts_wait.tv_sec++;
                    ts_wait.tv_nsec -= 1000000000;
                }

                // NEW: Atomically release mutex and wait
                // This is the core change for responsive waiting.
                printf("Scheduler waiting for %lld ms...\n", sleep_duration_ms);
                pthread_cond_timedwait(&s_scheduler_cond, &s_scheduler_mutex, &ts_wait);
                // When pthread_cond_timedwait returns, the mutex is re-acquired.
                printf("Scheduler woken up.\n");

            } else {
                // Next task is already due or overdue, no sleep needed.
                // Just release lock and loop immediately to process.
                 pthread_mutex_unlock(&s_scheduler_mutex);
                 continue; // Continue outer while(1) loop to re-evaluate tasks
            }
        } else {
            // No more tasks in the scheduler.
            printf("No more tasks in the scheduler. Exiting run loop.\n");
            pthread_mutex_unlock(&s_scheduler_mutex); // Release lock before breaking
            break;
        }

        pthread_mutex_unlock(&s_scheduler_mutex); // Release lock before next iteration of outer loop
    }
}

void scheduler_shutdown() {
    printf("Scheduler shutting down.\n");

    pthread_mutex_lock(&s_scheduler_mutex); // Acquire lock for cleanup

    for (size_t i = 0; i < s_task_heap.size; ++i) {
        free(s_task_heap.tasks[i]);
    }
    free(s_task_heap.tasks);
    s_task_heap.tasks = NULL;
    s_task_heap.capacity = 0;
    s_task_heap.size = 0;

    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        s_task_id_map[i].occupied = false;
        s_task_id_map[i].id = INVALID_TASK_ID;
    }
    s_map_size = 0;

    pthread_mutex_unlock(&s_scheduler_mutex); // Release lock

    pthread_mutex_destroy(&s_scheduler_mutex);
    pthread_cond_destroy(&s_scheduler_cond); // NEW: Destroy cond var

    printf("All remaining tasks and scheduler memory freed. Mutex and Condition Variable destroyed.\n");
}