#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "scheduler.h"

// Task functions
void my_task_function(void *arg);
void periodic_task_func(void *arg);
void cancellation_task(void *arg);
void *thread_func(void *arg);

#endif
