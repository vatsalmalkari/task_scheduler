#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "scheduler.h" // Include scheduler.h if your tasks interact with the scheduler (e.g., cancellation_task)

// task functions here
void my_one_shot_task(void *arg);
void my_task_function(void *arg);
void my_periodic_task(void *arg);
void my_cleanup_task(void *arg);
void cancellation_task(void *arg);
void another_task(void *arg);
void periodic_task_func(void *arg);
void *thread_func(void *arg); // Thread function to schedule tasks from a separate thread
// Function to run a periodic task
void run_periodic_task(void *arg);
// Function to schedule tasks for testing
void schedule_test_tasks();
// Function to cancel tasks for testing
void cancel_test_tasks();
// Function to run the application tasks
void run_app_tasks();
// Function to run the application tasks in a separate thread
void *run_app_tasks_thread(void *arg);
// Function to run the application tasks in a separate thread with cancellation
void *run_app_tasks_thread_with_cancellation(void *arg);
// Function to run the application tasks in a separate thread with cancellation and cleanup
void *run_app_tasks_thread_with_cancellation_and_cleanup(void *arg);
// Function to run the application tasks in a separate thread with cancellation, cleanup, and periodic tasks
void *run_app_tasks_thread_with_cancellation_cleanup_and_periodic(void *arg);
// Function to run the application tasks in a separate thread with cancellation, cleanup, periodic tasks, and one-shot tasks
void *run_app_tasks_thread_with_cancellation_cleanup_periodic_and_oneshot(void *arg);
// Function to run the application tasks in a separate thread with cancellation, cleanup, periodic tasks, one-shot tasks, and task scheduling
void *run_app_tasks_thread_with_cancellation_cleanup_periodic_oneshot_and_scheduling(void*arg);
// Function to run the application tasks in a separate thread with cancellation, cleanup, periodic tasks, one-shot tasks, task scheduling, and task cancellation
void *run_app_tasks_thread_with_cancellation_cleanup_periodic_oneshot_scheduling_and_cancellation(void*arg);
// Function to run the application tasks in a separate thread with cancellation, cleanup, periodic tasks, one-shot tasks, task scheduling, task cancellation, and task rescheduling
void *run_app_tasks_thread_with_cancellation_cleanup_periodic_oneshot_scheduling_cancellation_and_rescheduling(void*arg);
// Function to run the application tasks in a separate thread with cancellation, cleanup, periodic tasks, one-shot tasks, task scheduling, task cancellation, task rescheduling, and task cleanup
void *run_app_tasks_thread_with_cancellation_cleanup_periodic_oneshot_scheduling_cancellation_rescheduling_and_cleanup(void*arg);
// Function to run the application tasks in a separate thread with cancellation, cleanup, periodic tasks, one-shot tasks, task scheduling, task cancellation, task rescheduling, task cleanup, and task rescheduling
void *run_app_tasks_thread_with_cancellation_cleanup_periodic_oneshot_scheduling_cancellation_rescheduling_cleanup_and_rescheduling(void*arg);
// Function to run the application tasks in a separate thread with cancellation, cleanup, periodic tasks, one-shot tasks, task scheduling, task cancellation, task rescheduling, task cleanup, task rescheduling, and task scheduling
void *run_app_tasks_thread_with_cancellation_cleanup_periodic_oneshot_scheduling_cancellation_rescheduling_cleanup_rescheduling_and_scheduling(void*arg);
// Function to run the application tasks in a separate thread with cancellation, cleanup, periodic tasks, one-shot tasks, task scheduling, task cancellation, task rescheduling, task cleanup, task rescheduling, task scheduling, and task cancellation
void *run_app_tasks_thread_with_cancellation_cleanup_periodic_oneshot_scheduling_cancellation_rescheduling_cleanup_rescheduling_scheduling_and_cancellation(void*arg);
#endif // APP_TASKS_H