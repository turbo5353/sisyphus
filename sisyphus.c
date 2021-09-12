#include <gtk/gtk.h> 
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    size_t task_len;
    char *task_str;
} Task;

Task task_new(void) {
    Task task;
    task.task_str = NULL;
    task.task_len = 0;
    return task;
}

void set_task_str(Task *task, char *str) {
    size_t len = strlen(str);
    if (len > task->task_len) {
        task->task_str = (char*) realloc(task->task_str, len * sizeof(char));
    }

    strcpy(task->task_str, str);
}

unsigned int g_num_tasks = 0;
Task *task_list = NULL;

void allocate_task_list(unsigned int num_tasks) {
    if (num_tasks > g_num_tasks && num_tasks > 0) {
        g_num_tasks = num_tasks;
        task_list = realloc(task_list, g_num_tasks * sizeof(Task));
    }
}

static void build_ui(GtkApplication *app) {
    // Create window
    GtkWidget *window = gtk_application_window_new(app);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_container_add(GTK_CONTAINER(window), box);

    for (unsigned int i = 0; i < g_num_tasks; i++) {
        GtkWidget *check = gtk_check_button_new_with_label(task_list[i].task_str);
        gtk_container_add(GTK_CONTAINER(box), check);
    }

    // Show the window and all its children
    gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char *argv[]) {
    allocate_task_list(4);

    task_list[0] = task_new();
    set_task_str(&task_list[0], "task 1");

    task_list[1] = task_new();
    set_task_str(&task_list[1], "(A) task 2");

    task_list[2] = task_new();
    set_task_str(&task_list[2], "(B) 2021-09-12 task3");

    task_list[3] = task_new();
    set_task_str(&task_list[3], "x 2021-09-12 2021-09-12 task 4");

    // Create GtkApplication
    GtkApplication *app = gtk_application_new("xyz.fossible.sisyphus", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(build_ui), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}

