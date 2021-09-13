#include "tasks.h"
#include <string.h>

unsigned int g_num_tasks = 0;
Task *task_list = NULL;

Task task_new(void) {
    Task task;
    task.checked = 0;
    task.task_str = NULL;
    task.task_len = 0;
    return task;
}

void set_task_str(Task *task, char *str) {
    size_t len = strlen(str);
    if (len > task->task_len) {
        task->task_str = (char*) realloc(task->task_str, len * sizeof(char));
        task->task_len = len;
    }

    strcpy(task->task_str, str);
}

void allocate_task_list(unsigned int num_tasks) {
    if (num_tasks > g_num_tasks && num_tasks > 0) {
        g_num_tasks = num_tasks;
        task_list = (Task*)realloc(task_list, g_num_tasks * sizeof(Task));
    }
}

