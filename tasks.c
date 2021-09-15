#include "tasks.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <glib.h>

#define DEFAULT_TASK_LIST_SIZE 2

unsigned int g_max_tasks = DEFAULT_TASK_LIST_SIZE;
unsigned int g_num_tasks = 0;
Task *task_list = NULL;

Task task_new(void) {
    Task task;
    task.checked = 0;
    task.priority = 0;
    task.creation_day = 1;
    task.creation_month = 0;
    task.creation_year = 1000;
    task.description = NULL;
    task.desc_len = 0;
    return task;
}

void set_task_description(Task *task, const char *str) {
    size_t len = strlen(str);
    if (len > task->desc_len) {
        task->description = (char*) realloc(task->description, len * sizeof(char));
        task->desc_len = len;
    }

    strcpy(task->description, str);
}

void set_creation_time_now(Task *task) {
    time_t raw = time(NULL);
    struct tm *info = localtime(&raw);
    task->creation_day = info->tm_mday;
    task->creation_month = info->tm_mon;
    task->creation_year = info->tm_year + 1900;
}

char* get_task_priority_string(Task *task) {
    // priority colors
    const unsigned int num_colors = 5;
    const char *priority_colors[] = { "red", "orange", "green", "blue", "gray" };

    // priority pango markup string
    const char *priority_format = "<span foreground=\"%s\">%c</span>";

    char priority_letter = '-';
    const char *color = "gray";

    if (task->priority) {
        // 65 is the index of the 'A' character
        priority_letter = task->priority + 64;

        if (task->priority > num_colors) {
            color = priority_colors[num_colors - 1];
        }
        else {
            color = priority_colors[task->priority - 1];
        }
    }

    // the 7 makes space for the color string (e.g #000000)
    size_t size = strlen(priority_format) + 7;
    char *res = malloc(size * sizeof(char));

    snprintf(res, size * sizeof(char), priority_format, color, priority_letter);
    return res;
}

char* get_task_display_string(Task *task) {
    gchar *escaped = g_markup_escape_text(task->description, -1);

    char *res = malloc(strlen(escaped) * sizeof(char));
    strcpy(res, escaped);

    g_free(escaped);
    return res;
}

void init_task_list() {
    task_list = (Task*)malloc(DEFAULT_TASK_LIST_SIZE * sizeof(Task));
}

Task* add_task() {
    if (g_num_tasks >= g_max_tasks) {
        g_max_tasks *= 2;
        task_list = (Task*)realloc(task_list, g_max_tasks * sizeof(Task));
    }

    g_num_tasks++;
    task_list[g_num_tasks - 1] = task_new();
    return &task_list[g_num_tasks - 1];
}

void remove_task(unsigned int index) {
    free(task_list[index].description);
    g_num_tasks--;

    if (index < g_num_tasks) {
        memmove(task_list + index, task_list + index + 1, (g_num_tasks - index) * sizeof(Task));
    }
}

