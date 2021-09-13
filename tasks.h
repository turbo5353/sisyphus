#ifndef TASKS_H
#define TASKS_H

#include <stdlib.h>

typedef struct {
    char checked;
    size_t desc_len;
    char *description;
} Task;

Task task_new(void);
void set_task_description(Task *task, char *str);
char* get_task_display_string(Task *task);

extern unsigned int g_num_tasks;
extern Task *task_list;

void allocate_task_list(unsigned int num_tasks);

#endif

