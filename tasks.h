#ifndef TASKS_H
#define TASKS_H

#include <stdlib.h>

typedef struct {
    char checked;
    char priority;
    size_t desc_len;
    char *description;
} Task;

Task task_new(void);
void set_task_description(Task *task, const char *str);
char* get_task_display_string(Task *task);

extern unsigned int g_num_tasks;
extern Task *task_list;

void init_task_list();
Task* add_task();

#endif

