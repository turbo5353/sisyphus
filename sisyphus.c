#include <gtk/gtk.h> 
#include <stdio.h>
#include <stdlib.h>

#include "tasks.h"

GtkWidget* create_task_element(Task task) {
    GtkWidget *task_element = gtk_check_button_new();

    if (strlen(task.task_str) >= 2) {
        if (strncmp(task.task_str, "x ", strlen("x ")) == 0) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_element), TRUE);
        }
    }

    gchar *escaped = g_markup_escape_text(task.task_str, -1);
    GtkWidget *label = gtk_label_new(escaped);
    g_free(escaped);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);

    gtk_container_add(GTK_CONTAINER(task_element), label);
    return task_element;
}

void task_toggled(GtkWidget *check, gpointer data);

GtkWidget *task_box = NULL;
void create_task_ui(void) {
    // Remove all previous children
    GList *list = NULL, *iterator = NULL;
    list = gtk_container_get_children(GTK_CONTAINER(task_box));
    for (iterator = list; iterator; iterator = iterator->next) {
        gtk_container_remove(GTK_CONTAINER(task_box), GTK_WIDGET(iterator->data));
    }

    for (unsigned int i = 0; i < g_num_tasks; i++) {
        GtkWidget *check = create_task_element(task_list[i]);
        g_signal_connect(check, "toggled", G_CALLBACK(task_toggled), (void*)(task_list + i));
        gtk_container_add(GTK_CONTAINER(task_box), check);
    }

    gtk_widget_show_all(task_box);
}

void task_toggled(GtkWidget *check, gpointer data) {
    Task *task = (Task*) data;

    if (strlen(task->task_str) >= 2) {
        if (strncmp(task->task_str, "x ", strlen("x ")) == 0) {
            memmove(task->task_str, task->task_str + 2, strlen(task->task_str) - 2);
        }
    }

    create_task_ui();
}

void build_ui(GtkApplication *app) {
    // Create window
    GtkWidget *window = gtk_application_window_new(app);

    task_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(task_box, 12);
    gtk_widget_set_margin_bottom(task_box, 12);
    gtk_widget_set_margin_start(task_box, 12);
    gtk_widget_set_margin_end(task_box, 12);
    gtk_container_add(GTK_CONTAINER(window), task_box);

    create_task_ui();

    // Show the window and all its children
    gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char *argv[]) {
    allocate_task_list(4);

    task_list[0] = task_new();
    set_task_str(&task_list[0], "task 1");

    task_list[1] = task_new();
    set_task_str(&task_list[1], "(A) <i>task 2</i>");

    task_list[2] = task_new();
    set_task_str(&task_list[2], "(B) 2021-09-12 task3");

    task_list[3] = task_new();
    set_task_str(&task_list[3], "x 2021-09-12 2021-09-12 task 4");

    // Create GtkApplication
    GtkApplication *app = gtk_application_new("xyz.fossible.sisyphus", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(build_ui), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}

