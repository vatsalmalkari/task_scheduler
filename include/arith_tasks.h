#ifndef ARITH_TASKS_H
#define ARITH_TASKS_H
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

// Arithmetic Task Struct
typedef struct {
    int a;
    int b;
    char op; // '+', '-', '*', '/'
} arithmetic_task_t;
// This generates random arithmetic task
static inline arithmetic_task_t* make_random_task() {
    arithmetic_task_t *task = malloc(sizeof(arithmetic_task_t));
    task->a = rand() % 100;
    task->b = rand() % 100;
    char ops[] = {'+', '-', '*', '/'};
    task->op = ops[rand() % 4];
    return task;
}
// This execute arithmetic task
static inline void execute_task(arithmetic_task_t *task) {
    int result = 0;
    switch(task->op) {
        case '+': result = task->a + task->b; break;
        case '-': result = task->a - task->b; break;
        case '*': result = task->a * task->b; break;
        case '/': result = task->b != 0 ? task->a / task->b : 0; break;
    }
    printf("[Arithmetic] %d %c %d = %d\n", task->a, task->op, task->b, result);
    
}
#endif
