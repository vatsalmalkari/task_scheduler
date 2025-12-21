# Lightweight, Event-Driven Task Scheduler in C

# Overview
This project is a thread-safe, high-performance task scheduler library in C. It lets your applications run functions (tasks) once after a delay or repeatedly at fixed intervals. Designed for systems programming, it's ideal for embedded systems, game servers, or high-performance backend services.

##  Features
**One-Shot Tasks**: Run a function once after a specified delay.

**Periodic Tasks**: Run functions repeatedly at fixed intervals.

**Task Cancellation**: Remove scheduled tasks by ID, even from other threads.

**Efficient Scheduling**: Uses a min-heap for O(log N) scheduling and retrieval.

**Fast Lookup**: Hash table maps task IDs to heap positions for near O(1) cancellation.

**Thread-Safe**: Protected with pthread_mutex_t and pthread_cond_t—safe for multi-threaded use.

**High-Resolution Timing**: Uses CLOCK_MONOTONIC for reliable timing, independent of system clock changes.

**Graceful Shutdown**: Frees all memory and system resources when finished.

**Stress-Tested**: Handles thousands of tasks safely with logging and race-condition checks.

##  Build Instructions
Getting the scheduler up and running

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/vatsalmalkari/task_scheduler.git
    cd task_scheduler

    ```
2.  **Build the project:**
    ```bash
    make clean
    make
    ```
    This command compiles the scheduler library and the example application, creating the `scheduler_app` executable in the root directory.

#  How to Run the Example
Once built, you can run the demonstration application to see the scheduler in action:

```bash
./scheduler_app
```

# Example usage
```bash
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>

void my_task(void *arg) {
    printf("Task executed with arg=%d\n", *(int*)arg);
    }

int main() {
    scheduler_init();

    int *value = malloc(sizeof(int));
    *value = 42;
    
    // Schedule a one-shot task to run after 500ms
    schedule_once(500, my_task, value, true);

    // Run scheduler in main thread
    scheduler_run();

    scheduler_shutdown();
    return 0;
}
```


**Why Use This Scheduler?**

**Lightweight and easy to integrate into C projects.**

**Handles thousands of tasks efficiently.**

**Safe for multi-threaded programs.**

**Minimal CPU usage, avoiding busy-waiting.**

**Great for high-performance systems.**