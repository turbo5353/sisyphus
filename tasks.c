#include "tasks.h"
#include <string.h>

unsigned int g_num_tasks = 0;
Task *task_list = NULL;

Task task_new(void) {
    Task task;
    task.checked = 0;
    task.description = NULL;
    task.desc_len = 0;
    return task;
}

void set_task_description(Task *task, char *str) {
    size_t len = strlen(str);
    if (len > task->desc_len) {
        task->description = (char*) realloc(task->description, len * sizeof(char));
        task->desc_len = len;
    }

    strcpy(task->description, str);
}

char* get_task_display_string(Task *task) {
    char* res = malloc(task->desc_len * sizeof(char));
    strcpy(res, task->description);
    return res;
}

void allocate_task_list(unsigned int num_tasks) {
    if (num_tasks > g_num_tasks && num_tasks > 0) {
        g_num_tasks = num_tasks;
        task_list = (Task*)realloc(task_list, g_num_tasks * sizeof(Task));
    }
}

