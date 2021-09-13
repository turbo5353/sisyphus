#include "tasks.h"
#include <stdio.h>
#include <string.h>
#include <glib.h>

unsigned int g_num_tasks = 0;
Task *task_list = NULL;

Task task_new(void) {
    Task task;
    task.checked = 0;
    task.priority = 0;
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
    // priority colors
    const unsigned int num_colors = 5;
    const char *priority_colors[] = { "red", "orange", "green", "blue", "gray" };

    // priority pango markup string
    const char *priority_format = "<span foreground=\"%s\">(%c)</span> ";

    gchar *escaped = g_markup_escape_text(task->description, -1);
    char *res = NULL;

    if (task->priority) {
        // 65 is the index of the 'A' character
        char priority_letter = task->priority + 64;

        // the 7 makes space for the color string
        size_t size = strlen(escaped) + strlen(priority_format) + 7;
        res = malloc(size * sizeof(char));

        const char *color;
        if (task->priority > num_colors) {
            color = priority_colors[num_colors - 1];
        }
        else {
            color = priority_colors[task->priority - 1];
        }

        int offset = snprintf(res, size, priority_format, color, priority_letter);
        strcpy(res + offset, escaped);
    }
    else {
        res = malloc(strlen(escaped) * sizeof(char));
        strcpy(res, escaped);
    }

    g_free(escaped);
    return res;
}

void allocate_task_list(unsigned int num_tasks) {
    if (num_tasks > g_num_tasks && num_tasks > 0) {
        g_num_tasks = num_tasks;
        task_list = (Task*)realloc(task_list, g_num_tasks * sizeof(Task));
    }
}

