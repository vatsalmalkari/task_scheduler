#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#define INITIAL_HEAP_CAPACITY 10
#define HASH_TABLE_CAPACITY 3000

#define DEBUG 0
#define DEBUG_LOG(fmt, ) \
    do { if (DEBUG) { pthread_mutex_lock(&log_mutex); \
        printf(fmt "\n", ##__VA_ARGS__); \
        pthread_mutex_unlock(&log_mutex); } } while (0)

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

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
static size_t s_map_size = 0;

static pthread_mutex_t s_mutex;
static pthread_cond_t s_cond;

static uint64_t now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void swap(size_t i, size_t j) {
    if (i == j) return;

    scheduled_task_t *tmp = s_heap.tasks[i];
    s_heap.tasks[i] = s_heap.tasks[j];
    s_heap.tasks[j] = tmp;

    s_map[s_heap.tasks[i]->id % HASH_TABLE_CAPACITY].heap_idx = i;
    s_map[s_heap.tasks[j]->id % HASH_TABLE_CAPACITY].heap_idx = j;

    DEBUG_LOG("[heap] Swapped %zu <-> %zu", i, j);
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

        if (left < s_heap.size &&
            s_heap.tasks[left]->execute_time_ms < s_heap.tasks[smallest]->execute_time_ms)
            smallest = left;

        if (right < s_heap.size &&
            s_heap.tasks[right]->execute_time_ms < s_heap.tasks[smallest]->execute_time_ms)
            smallest = right;

        if (smallest != idx) {
            swap(idx, smallest);
            idx = smallest;
        } else break;
    }
}

static int ensure_capacity() {
    if (s_heap.size < s_heap.capacity)
        return 0;

    size_t new_cap = s_heap.capacity == 0 ? INITIAL_HEAP_CAPACITY : s_heap.capacity * 2;
    scheduled_task_t **new_ptr =
        realloc(s_heap.tasks, new_cap * sizeof(scheduled_task_t *));

    if (!new_ptr) return -1;

    s_heap.tasks = new_ptr;
    s_heap.capacity = new_cap;

    DEBUG_LOG("[heap] Resized to %zu", new_cap);
    return 0;
}

static size_t hash(task_id_t id) { 
    return id % HASH_TABLE_CAPACITY; 
}

static void map_insert(task_id_t id, size_t heap_idx) {
    size_t h = hash(id);
    while (s_map[h].occupied)
        h = (h + 1) % HASH_TABLE_CAPACITY;
    s_map[h].id = id;
    s_map[h].heap_idx = heap_idx;
    s_map[h].occupied = true;
    s_map_size++;
    DEBUG_LOG("[map] Inserted %u @ %zu", id, heap_idx);
}

static bool map_get(task_id_t id, size_t *out_idx) {
    size_t h = hash(id);
    size_t start = h;

    while (s_map[h].occupied) {
        if (s_map[h].id == id) {
            *out_idx = s_map[h].heap_idx;
            return true;
        }
        h = (h + 1) % HASH_TABLE_CAPACITY;
        if (h == start) break;
    }
    return false;
}

static void map_remove(task_id_t id) {
    size_t h = hash(id);
    size_t start = h;

    while (s_map[h].occupied) {
        if (s_map[h].id == id) {
            s_map[h].occupied = false;
            s_map_size--;
            DEBUG_LOG("[map] Removed %u", id);
            return;
        }
        h = (h + 1) % HASH_TABLE_CAPACITY;
        if (h == start) break;
    }
}

static void free_task(scheduled_task_t *t) {
    if (!t) return;

    if (t->arg_is_heap && t->arg) {
        DEBUG_LOG("[free] Freeing arg of %u", t->id);
        free(t->arg);
    }

    DEBUG_LOG("[free] Freeing task %u", t->id);
    free(t);
}

static task_id_t make_task(uint64_t delay_ms, task_func_t func, void *arg, bool periodic, uint64_t interval, bool arg_is_heap)
{
    if (!func) return INVALID_TASK_ID;

    pthread_mutex_lock(&s_mutex);
    if (ensure_capacity() != 0) {
        pthread_mutex_unlock(&s_mutex);
        return INVALID_TASK_ID;
    }
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
    pthread_cond_signal(&s_cond);
    pthread_mutex_unlock(&s_mutex);

    DEBUG_LOG("[schedule] id=%u delay=%llu", t->id, delay_ms);
    return t->id;
}


void scheduler_init() {
    pthread_mutex_init(&s_mutex, NULL);
    pthread_cond_init(&s_cond, NULL);
    s_heap.tasks = calloc(INITIAL_HEAP_CAPACITY, sizeof(scheduled_task_t *));
    s_heap.capacity = INITIAL_HEAP_CAPACITY;
    s_heap.size = 0;
    s_next_task_id = 1;
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; i++)
        s_map[i].occupied = false;
    s_map_size = 0;
    DEBUG_LOG("init Scheduler ready");
}

task_id_t schedule_once(uint64_t delay_ms, task_func_t f, void *arg, bool heap_arg) {
    return make_task(delay_ms, f, arg, false, 0, heap_arg);
}

task_id_t schedule_periodic(uint64_t delay_ms, uint64_t interval_ms,
                            task_func_t f, void *arg, bool heap_arg) {
    if (interval_ms == 0)
        return schedule_once(delay_ms, f, arg, heap_arg);

    return make_task(delay_ms, f, arg, true, interval_ms, heap_arg);
}

int cancel_task(task_id_t id) {
    if (id == INVALID_TASK_ID) return -1;

    pthread_mutex_lock(&s_mutex);

    size_t idx;
    if (!map_get(id, &idx)) {
        pthread_mutex_unlock(&s_mutex);
        return -1;
    }

    s_heap.tasks[idx]->cancelled = true;
    DEBUG_LOG("[cancel] %u marked cancelled", id);

    pthread_mutex_unlock(&s_mutex);
    return 0;
}

static scheduled_task_t *pop_top() {
    if (s_heap.size == 0) return NULL;

    scheduled_task_t *t = s_heap.tasks[0];

    s_heap.size--;

    if (s_heap.size > 0) {
        s_heap.tasks[0] = s_heap.tasks[s_heap.size];
        map_insert(s_heap.tasks[0]->id, 0);
        heapify_down(0);
    }

    map_remove(t->id);

    DEBUG_LOG("[pop] %u", t->id);
    return t;
}


void scheduler_run() {
    for (;;) {
        pthread_mutex_lock(&s_mutex);
        uint64_t now = now_ms();
    
        while (s_heap.size > 0 &&
               s_heap.tasks[0]->execute_time_ms <= now)
        {
            scheduled_task_t *t = pop_top();
            pthread_mutex_unlock(&s_mutex);
            if (!t->cancelled && t->func) {
                DEBUG_LOG("[run] Executing %u", t->id);
                t->func(t->arg);
            }
            pthread_mutex_lock(&s_mutex);
            if (t->is_periodic && !t->cancelled) {
                t->execute_time_ms = now_ms() + t->interval_ms;
                s_heap.tasks[s_heap.size] = t;
                map_insert(t->id, s_heap.size);
                heapify_up(s_heap.size);
                s_heap.size++;
                DEBUG_LOG("[run] Rescheduled %u", t->id);
            } else {
                free_task(t);
            }
            now = now_ms();
        }
        if (s_heap.size == 0) {
            pthread_mutex_unlock(&s_mutex);
            return; 
        }
        uint64_t sleep_ms = s_heap.tasks[0]->execute_time_ms - now;
        if ((long long)sleep_ms > 0) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            ts.tv_sec += sleep_ms / 1000;
            ts.tv_nsec += (sleep_ms % 1000) * 1000000;
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000;
            }
            pthread_cond_timedwait(&s_cond, &s_mutex, &ts);
        }
        pthread_mutex_unlock(&s_mutex);
    }
}

void scheduler_shutdown() {
    pthread_mutex_lock(&s_mutex);
    for (size_t i = 0; i < s_heap.size; i++)
        free_task(s_heap.tasks[i]);
    free(s_heap.tasks);
    s_heap.tasks = NULL;
    s_heap.size = 0;
    s_heap.capacity = 0;
    pthread_mutex_unlock(&s_mutex);
    pthread_mutex_destroy(&s_mutex);
    pthread_cond_destroy(&s_cond);
    DEBUG_LOG("shutdown complete");
}
