#ifndef TASKS_H
#define TASKS_H

#include <stdlib.h>

typedef struct {
    char checked;
    size_t task_len;
    char *task_str;
} Task;

Task task_new(void);
void set_task_str(Task *task, char *str);

extern unsigned int g_num_tasks;
extern Task *task_list;

void allocate_task_list(unsigned int num_tasks);

#endif

