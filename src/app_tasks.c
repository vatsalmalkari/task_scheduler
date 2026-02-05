#include "app_tasks.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void my_task_function(void *arg) {
    printf("EXEC One-shot Task | Arg: %s\n", (char *)arg);
}

void periodic_task_func(void *arg) {
    printf("EXEC Periodic Task Heartbeat | Arg: %s\n", (char*)arg);
}

void *thread_func(void *arg) {
    (void)arg;
  
    usleep(1000000); 
    printf("THREAD Background thread injecting new task \n");
    schedule_once(100, my_task_function, "Dynamic Thread Task", false);
    return NULL;
}
void cancel_task_wrapper(void *arg) {
    task_id_t id = (task_id_t)(uintptr_t)arg;
    cancel_task(id); 
    printf("WRAPPER Task %u has been cancelled via scheduled event.\n", id);
}

void cancellation_edge_test() {
    printf("\n STARTING CANCELLATION STRESS TEST \n");
    scheduler_init();

    // 1. Start the Scheduler in its own thread
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, (void*(*)(void*))scheduler_run, NULL);

    const int TOTAL_TASKS = 100;
    task_id_t ids[TOTAL_TASKS];

    // 2. Schedule tasks (100ms delay)
    for(int i = 0; i < TOTAL_TASKS; i++) {
        ids[i] = schedule_once(100, my_task_function, "Canceller Test", false);
    }

    // 3. immediately cancel even-indexed tasks
    int success = 0;
    for(int i = 0; i < TOTAL_TASKS; i += 2) {
        if(cancel_task(ids[i]) == 0) success++;
    }

    printf("INFO Successfully cancelled %d/50 tasks before they could run.\n", success);
    
    // 4. Sleep to let the remaining 50 tasks execute
    usleep(200000); 

    // 5. Shutdown Sequence (Order is critical!)
    printf("INFO Shutting down...\n");
    
    scheduler_shutdown();                 
    pthread_join(scheduler_thread, NULL); 

    printf("CANCELLATION TEST COMPLETE\n");
    fflush(stdout);
}

int scheduler_test_main() {
    printf("\n INTEGRATION TEST: Task Subsystem \n");
    scheduler_init();

    // 1. Test One-shot Execution
    schedule_once(500, my_task_function, "500ms Delay", false);

    // 2. Test Periodic Logic (Regression Test)
    task_id_t p_id = schedule_periodic(200, 1000, periodic_task_func, "1s Interval", false);

    // 3. Test Thread Safety
    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, NULL);
    pthread_detach(thread);

    // 4. Test Cancellation
    task_id_t *id_to_kill = malloc(sizeof(task_id_t));
    *id_to_kill = p_id;
    schedule_once(3500, cancel_task_wrapper, (void*)(uintptr_t)p_id, false);

    printf("INFO System running. Observe task timing and thread injection...\n");
    
    scheduler_run(NULL);

    scheduler_shutdown();
    printf("INTEGRATION TEST DONE\n");
    return 0;
}