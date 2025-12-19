# A Lightweight, Event-Driven Task Scheduler for C

---

##  Overview

This project provides a robust, thread-safe, and efficient task scheduler library written in C. It empowers your applications to execute functions (tasks) precisely—either once after a specified delay or repeatedly at fixed intervals. Designed with systems programming in mind, it's perfect for scenarios where precise timing and concurrent task management are critical, such as in embedded systems, game servers, or high-performance backend services.

##  Features

* **One-Shot Tasks:** Schedule functions to run a single time after a defined delay.
* **Periodic Tasks:** Set up functions to execute repeatedly at consistent intervals.
* **Task Cancellation:** Easily remove scheduled tasks by their unique ID, even from other threads.
* **Efficient Time Management:** Uses a **min-heap** (a type of priority queue) to quickly determine the next task to run. This ensures $O(\log N)$ efficiency for scheduling and retrieving tasks, even with many concurrent operations.
* **Fast Task Lookup:** Employs a **hash table** to map task IDs to their location in the heap, allowing for near $O(1)$ average-case performance when canceling tasks.
* **Thread-Safe Operations:** All scheduler functions are protected by `pthread_mutex_t` and `pthread_cond_t`. This means you can safely schedule or cancel tasks from multiple threads without worrying about data corruption or race conditions.
* **High-Resolution Timing:** Leverages `clock_gettime(CLOCK_MONOTONIC)` for accurate, system-time-independent scheduling, ensuring tasks execute reliably even if the system clock changes.
* **Graceful Shutdown:** Cleans up all dynamically allocated memory and system resources (like mutexes and condition variables) when the scheduler is no longer needed.

---

##  Build Instructions

Getting the scheduler up and running is straightforward.

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

---

##  How to Run the Example

Once built, you can run the demonstration application to see the scheduler in action:

```bash
./scheduler_app
