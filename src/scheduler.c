#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define INITIAL_HEAP_CAPACITY 10
#define HASH_TABLE_CAPACITY 4000 
static volatile int s_running = 1;

#define DEBUG 1
#define DEBUG_LOG(fmt, ...) \
    do { if (DEBUG) { pthread_mutex_lock(&log_mutex); \
        printf(fmt "\n", ##__VA_ARGS__); \
        pthread_mutex_unlock(&log_mutex); } } while (0)

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    uint64_t tasks_scheduled;
    uint64_t tasks_executed;
    uint64_t tasks_cancelled;
    uint64_t total_drift_ms;
    uint64_t max_drift_ms;
} scheduler_metrics_t;

typedef struct {
    scheduled_task_t **tasks;
    size_t size;
    size_t capacity;
} task_heap_t;

typedef struct {
    task_id_t id;
    size_t heap_idx;
    bool occupied;
} hash_entry_t;

static task_heap_t s_heap;
static task_id_t s_next_task_id = 1;
static hash_entry_t s_map[HASH_TABLE_CAPACITY];
static pthread_mutex_t s_mutex;
static pthread_cond_t s_cond;
static scheduler_metrics_t s_metrics = {0};

static uint64_t now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static size_t find_map_slot(task_id_t id) {
    size_t h = id % HASH_TABLE_CAPACITY;
    size_t start = h;
    while (s_map[h].occupied) {
        if (s_map[h].id == id) return h;
        h = (h + 1) % HASH_TABLE_CAPACITY;
        if (h == start) break;
    }
    return (size_t)-1;
}

static void swap(size_t i, size_t j) {
    if (i == j) return;

    scheduled_task_t *t_i = s_heap.tasks[i];
    scheduled_task_t *t_j = s_heap.tasks[j];

    s_heap.tasks[i] = t_j;
    s_heap.tasks[j] = t_i;

    size_t slot_i = find_map_slot(t_i->id);
    size_t slot_j = find_map_slot(t_j->id);
    
    if (slot_i != (size_t)-1) s_map[slot_i].heap_idx = j;
    if (slot_j != (size_t)-1) s_map[slot_j].heap_idx = i;
}

static void heapify_up(size_t idx) {
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (s_heap.tasks[idx]->execute_time_ms < s_heap.tasks[parent]->execute_time_ms) {
            swap(idx, parent);
            idx = parent;
        } else break;
    }
}

static void heapify_down(size_t idx) {
    for (;;) {
        size_t left = 2 * idx + 1;
        size_t right = left + 1;
        size_t smallest = idx;

        if (left < s_heap.size && s_heap.tasks[left]->execute_time_ms < s_heap.tasks[smallest]->execute_time_ms)
            smallest = left;
        if (right < s_heap.size && s_heap.tasks[right]->execute_time_ms < s_heap.tasks[smallest]->execute_time_ms)
            smallest = right;

        if (smallest != idx) {
            swap(idx, smallest);
            idx = smallest;
        } else break;
    }
}

static int ensure_capacity() {
    if (s_heap.size < s_heap.capacity) return 0;
    size_t new_cap = s_heap.capacity == 0 ? INITIAL_HEAP_CAPACITY : s_heap.capacity * 2;
    scheduled_task_t **new_ptr = realloc(s_heap.tasks, new_cap * sizeof(scheduled_task_t *));
    if (!new_ptr) return -1;
    s_heap.tasks = new_ptr;
    s_heap.capacity = new_cap;
    return 0;
}

static void map_insert(task_id_t id, size_t heap_idx) {
    size_t h = id % HASH_TABLE_CAPACITY;
    while (s_map[h].occupied) h = (h + 1) % HASH_TABLE_CAPACITY;
    s_map[h].id = id;
    s_map[h].heap_idx = heap_idx;
    s_map[h].occupied = true;
}

static void map_remove(task_id_t id) {
    size_t slot = find_map_slot(id);
    if (slot != (size_t)-1) s_map[slot].occupied = false;
}

static void free_task(scheduled_task_t *t) {
    if (!t) return;
    if (t->arg_is_heap && t->arg) free(t->arg);
    free(t);
}

void scheduler_init() {
    pthread_mutex_init(&s_mutex, NULL);
    pthread_cond_init(&s_cond, NULL);
    s_heap.tasks = calloc(INITIAL_HEAP_CAPACITY, sizeof(scheduled_task_t *));
    s_heap.capacity = INITIAL_HEAP_CAPACITY;
    s_heap.size = 0;
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; i++) s_map[i].occupied = false;
    DEBUG_LOG("[INIT] Scheduler Subsystem Online");
}

static task_id_t make_task(uint64_t delay_ms, task_func_t func, void *arg, bool periodic, uint64_t interval, bool arg_is_heap) {
    if (!func) return INVALID_TASK_ID;
    pthread_mutex_lock(&s_mutex);
    if (ensure_capacity() != 0) { pthread_mutex_unlock(&s_mutex); return INVALID_TASK_ID; }
    
    scheduled_task_t *t = calloc(1, sizeof(*t));
    t->id = s_next_task_id++;
    t->execute_time_ms = now_ms() + delay_ms;
    t->func = func;
    t->arg = arg;
    t->is_periodic = periodic;
    t->interval_ms = interval;
    t->arg_is_heap = arg_is_heap;

    s_heap.tasks[s_heap.size] = t;
    map_insert(t->id, s_heap.size);
    heapify_up(s_heap.size);
    s_heap.size++;
    s_metrics.tasks_scheduled++;

    pthread_cond_signal(&s_cond);
    pthread_mutex_unlock(&s_mutex);
    return t->id;
}

task_id_t schedule_once(uint64_t delay_ms, task_func_t f, void *arg, bool heap_arg) {
    return make_task(delay_ms, f, arg, false, 0, heap_arg);
}

task_id_t schedule_periodic(uint64_t delay_ms, uint64_t interval_ms, task_func_t f, void *arg, bool heap_arg) {
    return make_task(delay_ms, f, arg, true, interval_ms, heap_arg);
}

int cancel_task(task_id_t id) {
    pthread_mutex_lock(&s_mutex);
    size_t slot = find_map_slot(id);
    if (slot == (size_t)-1) { pthread_mutex_unlock(&s_mutex); return -1; }
    s_heap.tasks[s_map[slot].heap_idx]->cancelled = true;
    s_metrics.tasks_cancelled++;
    pthread_mutex_unlock(&s_mutex);
    return 0;
}

static scheduled_task_t *pop_top() {
    if (s_heap.size == 0) return NULL;
    scheduled_task_t *t = s_heap.tasks[0];
    map_remove(t->id);
    s_heap.size--;
    if (s_heap.size > 0) {
        s_heap.tasks[0] = s_heap.tasks[s_heap.size];
        size_t slot = find_map_slot(s_heap.tasks[0]->id);
        if (slot != (size_t)-1) s_map[slot].heap_idx = 0;
        heapify_down(0);
    }
    return t;
}

void* scheduler_run(void* arg) {
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&s_mutex);
        while (s_heap.size > 0) {
            uint64_t now = now_ms();
            if (s_heap.tasks[0]->execute_time_ms > now) break;

            scheduled_task_t *t = pop_top();
            uint64_t drift = now - t->execute_time_ms;
            
            s_metrics.tasks_executed++;
            s_metrics.total_drift_ms += drift;
            if (drift > s_metrics.max_drift_ms) s_metrics.max_drift_ms = drift;

            pthread_mutex_unlock(&s_mutex);
            if (!t->cancelled) {
                DEBUG_LOG("[EXEC] ID: %u | Drift: %llu ms", t->id, drift);
                t->func(t->arg);
            }

            pthread_mutex_lock(&s_mutex);
            if (t->is_periodic && !t->cancelled) {
                t->execute_time_ms = now_ms() + t->interval_ms;
                s_heap.tasks[s_heap.size] = t;
                map_insert(t->id, s_heap.size);
                heapify_up(s_heap.size);
                s_heap.size++;
            } else {
                free_task(t);
            }
        }

        if (s_heap.size == 0) {
            if (!s_running) {
                pthread_mutex_unlock(&s_mutex);
                return NULL;
            }
            pthread_cond_wait(&s_cond, &s_mutex);
            if (!s_running && s_heap.size == 0) {
                pthread_mutex_unlock(&s_mutex);
                return NULL;
            }
            continue;
        }

        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t wait_ms = s_heap.tasks[0]->execute_time_ms - now_ms();
        ts.tv_sec += wait_ms / 1000;
        ts.tv_nsec += (wait_ms % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
        
        pthread_cond_timedwait(&s_cond, &s_mutex, &ts);
        pthread_mutex_unlock(&s_mutex);
    }
    return NULL;
}

void scheduler_shutdown() {
    pthread_mutex_lock(&s_mutex);
    s_running = 0;
    pthread_cond_broadcast(&s_cond);

    printf("\n--- FINAL METRICS ---\n");
    printf("Tasks Executed: %llu\n", s_metrics.tasks_executed);
    printf("Max Drift: %llu ms\n", s_metrics.max_drift_ms);
    printf("Avg Drift: %.2f ms\n", (double)s_metrics.total_drift_ms / s_metrics.tasks_executed);
    
    for (size_t i = 0; i < s_heap.size; i++) free_task(s_heap.tasks[i]);
    free(s_heap.tasks);
    pthread_mutex_unlock(&s_mutex);
}

void print_final_metrics() {
    pthread_mutex_lock(&s_mutex);
    
    double avg = 0.0;
    if (s_metrics.tasks_executed > 0) {
        avg = (double)s_metrics.total_drift_ms / s_metrics.tasks_executed;
    }

    printf("\n PERFORMANCE SUMMARY \n");
    printf("  Tasks Executed:    %llu\n", (unsigned long long)s_metrics.tasks_executed);
    
    if (avg < 0.01) {
        printf("  Average Latency:   < 0.01 ms (Sub-millisecond)\n");
    } else {
        printf("  Average Latency:   %.4f ms\n", avg);
    }
    
    printf("  Worst-Case Jitter: %llu ms\n", (unsigned long long)s_metrics.max_drift_ms);
    
    pthread_mutex_unlock(&s_mutex);
}