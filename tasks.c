#include "tasks.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
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
    if (len >= task->desc_len) {
        task->description = (char*) realloc(task->description, (len + 1) * sizeof(char));
        task->desc_len = len + 1;
    }

    strcpy(task->description, str);
}

void set_task_creation_time_now(Task *task) {
    time_t raw = time(NULL);
    struct tm *info = localtime(&raw);
    task->creation_day = info->tm_mday;
    task->creation_month = info->tm_mon;
    task->creation_year = info->tm_year + 1900;
}

void set_task_completion_time_now(Task *task) {
    time_t raw = time(NULL);
    struct tm *info = localtime(&raw);
    task->completion_day = info->tm_mday;
    task->completion_month = info->tm_mon;
    task->completion_year = info->tm_year + 1900;
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

void write_file(const char *filename) {
    FILE *file = fopen(filename, "w");

    for (unsigned int i = 0; i < g_num_tasks; i++) {
        Task *task = &task_list[i];

        if (task->checked) {
            fputs("x ", file);
        }

        if (task->priority) {
            char priority_letter = task->priority + 64;
            fprintf(file, "(%c) ", priority_letter);
        }

        if (task->checked) {
            fprintf(file, "%04u-%02u-%02u ",
                    task->completion_year,
                    task->completion_month + 1,
                    task->completion_day);
        }

        fprintf(file, "%04u-%02u-%02u ",
                task->creation_year,
                task->creation_month + 1,
                task->creation_day);

        fputs(task->description, file);
        fputc('\n', file);
    }

    fclose(file);
}

bool read_checked(FILE *file) {
    char checked[3];
    fgets(checked, 3, file);

    if (strcmp("x ", checked) == 0) {
        return true;
    }
    else {
        fseek(file, -2, SEEK_CUR);
        return false;
    }
}

char read_priority(FILE *file) {
    char priority[4];
    fgets(priority, 4, file);

    if (priority[0] == '(' && priority[2] == ')') {
        if (priority[1] >= 'A' && priority[1] <= 'Z') {
            int next = fgetc(file);
            fseek(file, -1, SEEK_CUR);

            if (next == ' ' || next == '\n' || next == EOF) {
                return priority[1] - 64;
            }
        }
    }

    fseek(file, -3, SEEK_CUR);
    return 0;
}

bool read_date(FILE *file, int *day, int *month, int *year) {
    long cur = ftell(file);
    bool scan_success = fscanf(file, "%4d-%2d-%2d", year, month, day) == 3;
    int whitespace = fgetc(file);
    bool whitespace_success = whitespace == ' ' || whitespace == '\n' || whitespace == EOF;

    bool success = scan_success && whitespace_success;

    if (!success) {
        fseek(file, cur, SEEK_SET);
    }

    return success;
}

void read_dates(FILE *file, Task *task) {
    int first_day, first_month, first_year;
    bool first_date_success = read_date(file, &first_day, &first_month, &first_year);

    int second_day, second_month, second_year;
    bool second_date_success = read_date(file, &second_day, &second_month, &second_year);

    if (first_date_success && second_date_success) {
        // first date is the completion date
        // second date is the creation date
        if (task->checked) {
            task->completion_day = first_day;
            task->completion_month = first_month - 1;
            task->completion_year = first_year;
        }

        task->creation_day = second_day;
        task->creation_month = second_month - 1;
        task->creation_year = second_year;
    }
    else if (first_date_success) {
        // first date is the creation date
        // second date does not exist
        task->creation_day = first_day;
        task->creation_month = first_month - 1;
        task->creation_year = first_year;

        set_task_completion_time_now(task);
    }
    else {
        set_task_creation_time_now(task);
        set_task_completion_time_now(task);
    }
}

void read_file(const char *filename) {
    init_task_list();

    FILE *file = fopen(filename, "r");

    for (;;) {
        Task *task = add_task();

        if (fgetc(file) == '\n') {
            set_task_description(task, "");
            set_task_creation_time_now(task);
            set_task_completion_time_now(task);
            continue;
        }
        else {
            fseek(file, -1, SEEK_CUR);
        }

        task->checked = read_checked(file);
        task->priority = read_priority(file);
        read_dates(file, task);

        long cur = ftell(file);
        int next = fgetc(file);
        int num_chars = 0;
        while (next != '\n' && next != EOF) {
            num_chars++;
            next = fgetc(file);
        }

        if (num_chars > 0) {
            fseek(file, cur, SEEK_SET);
            char *desc = malloc((num_chars + 1) * sizeof(char));
            fgets(desc, num_chars + 1, file);

            set_task_description(task, desc);
            free(desc);
        }
        else {
            set_task_description(task, "");
        }

        if (next == '\n') fseek(file, 1, SEEK_CUR);
        if (fgetc(file) == EOF) {
            break;
        }
        else {
            fseek(file, -1, SEEK_CUR);
        }
    }

    fclose(file);
}

