#ifndef TASKS_H
#define TASKS_H

#include <stdlib.h>

typedef struct {
    char checked;
    char priority;

    char creation_day;
    char creation_month;
    int creation_year;

    char completion_day;
    char completion_month;
    int completion_year;

    size_t desc_len;
    char *description;
} Task;

Task task_new(void);
void set_task_description(Task *task, const char *str);
void set_task_creation_time_now(Task *task);
void set_task_completion_time_now(Task *task);
char* get_task_priority_string(Task *task);
char* get_task_display_string(Task *task);

extern unsigned int g_num_tasks;
extern Task *task_list;

void init_task_list();
Task* add_task();
void remove_task(unsigned int index);

#endif

