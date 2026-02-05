
---
# Lightweight Task Scheduler in C

A thread-safe, high-performance library for scheduling one-shot and periodic tasks. This project demonstrates advanced **Systems Programming** concepts including custom data structures, thread synchronization, and real-time performance metrics.

---

## Architecture & Efficiency

The scheduler is engineered for speed, using a hybrid data structure approach to ensure operations remain fast even as the number of tasks grows.

| Operation | Data Structure | Complexity | Why it matters |
| --- | --- | --- | --- |
| **Scheduling** | Min-Heap |  | Ensures the next due task is always at the top. |
| **Execution** | Min-Heap |  | Immediate access to the highest priority task. |
| **Cancellation** | Hash Map |  | Near-instant task removal by ID without scanning the list. |

---

## Key Features

* **Thread-Safe**: Uses Mutexes and Condition Variables to prevent data corruption in multi-threaded environments.
* **Precision Timing**: Utilizes `CLOCK_MONOTONIC` to prevent "drift" if the system clock changes.
* **No Busy-Waiting**: The worker thread sleeps until the exact moment a task is due, minimizing CPU usage.
* **Stress-Tested**: Validated with a 2,000-task concurrent load to ensure zero deadlocks or race conditions.

---

## Quick Start

### 1. Build the System

Ensure you have `gcc` and `make` installed.

```bash
make clean
make

```

### 2. Run Automated Validation (Stress Test)

This script simulates 2,000 tasks and mass cancellations to verify system integrity:

```bash
chmod +x test_suite.sh
./test_suite.sh

```

---

## Example Usage

This example shows how to initialize the scheduler, run it in a background thread, and schedule a task.

```c
#include "scheduler.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void my_task(void *arg) {
    printf("Task Executed: %s\n", (char*)arg);
}

int main() {
    scheduler_init();

    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, (void*(*)(void*))scheduler_run, NULL);

    schedule_once(500, my_task, "Hello World!", false);

    usleep(1000000); 

    scheduler_shutdown();
    pthread_join(scheduler_thread, NULL);

    return 0;
}

```

---

##  Performance Metrics

The system includes built-in reporting to measure scheduler overhead:

* **Average Latency**: Measures the accuracy of the firing time (Sub-millisecond resolution).
* **Throughput**: Validated to handle thousands of concurrent tasks with minimal jitter.
* **Memory Safety**: Zero leaks; all heap-allocated tasks are tracked and freed during shutdown.

---

## Project Structure

* `src/scheduler.c`: Core logic (Min-Heap, Hash Map, Thread Loop).
* `src/arithmetic_test.c`: Stress testing module for high-load validation.
* `test_suite.sh`: Automation script for end-to-end testing.
* `Makefile`: Handles clean builds and linking.

---