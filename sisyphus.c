#include <gtk/gtk.h> 
#include <stdio.h>
#include <stdlib.h>

#include "tasks.h"

GtkWidget* create_task_element(Task task) {
    GtkWidget *task_element = gtk_check_button_new();

    if (task.checked) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_element), TRUE);
    }

    char *display_str = get_task_display_string(&task);
    GtkWidget *label = gtk_label_new(display_str);
    free(display_str);

    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);

    gtk_container_add(GTK_CONTAINER(task_element), label);
    return task_element;
}

void task_toggled(GtkWidget *check, gpointer data) {
    Task *task = (Task*) data;
    task->checked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
}


GtkWidget *task_box = NULL;
void create_task_ui(const char *search) {
    // Remove all previous children
    GList *list = NULL, *iterator = NULL;
    list = gtk_container_get_children(GTK_CONTAINER(task_box));
    for (iterator = list; iterator; iterator = iterator->next) {
        gtk_container_remove(GTK_CONTAINER(task_box), GTK_WIDGET(iterator->data));
    }

    for (unsigned int i = 0; i < g_num_tasks; i++) {
        if (search) {
            if (!strstr(task_list[i].description, search)) {
                continue;
            }
        }

        GtkWidget *check = create_task_element(task_list[i]);
        g_signal_connect(check, "toggled", G_CALLBACK(task_toggled), (void*)(task_list + i));
        gtk_container_add(GTK_CONTAINER(task_box), check);
    }

    gtk_widget_show_all(task_box);
}

void search_changed(GtkSearchEntry *search_bar) {
    create_task_ui(gtk_entry_get_text(GTK_ENTRY(search_bar)));
}

void build_ui(GtkApplication *app) {
    // Create window
    GtkWidget *window = gtk_application_window_new(app);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget *search_bar = gtk_search_entry_new();
    g_signal_connect(search_bar, "search-changed", G_CALLBACK(search_changed), NULL);
    gtk_box_pack_start(GTK_BOX(box), search_bar, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    task_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(scroll), task_box);

    create_task_ui(NULL);

    // Show the window and all its children
    gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char *argv[]) {
    allocate_task_list(6);

    task_list[0] = task_new();
    task_list[0].priority = 2;
    set_task_description(&task_list[0], "task 1");

    task_list[1] = task_new();
    task_list[1].priority = 3;
    set_task_description(&task_list[1], "<i>task 2</i>");

    task_list[2] = task_new();
    task_list[2].priority = 4;
    set_task_description(&task_list[2], "task3");

    task_list[3] = task_new();
    task_list[3].checked = 1;
    task_list[3].priority = 1;
    set_task_description(&task_list[3], "task 4");

    task_list[4] = task_new();
    task_list[4].checked = 1;
    task_list[4].priority = 5;
    set_task_description(&task_list[4], "task 5");

    task_list[5] = task_new();
    task_list[5].checked = 1;
    task_list[5].priority = 6;
    set_task_description(&task_list[5], "task 6");

    // Create GtkApplication
    GtkApplication *app = gtk_application_new("xyz.fossible.sisyphus", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(build_ui), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}

