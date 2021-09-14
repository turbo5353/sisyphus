#include <gtk/gtk.h> 
#include <stdio.h>
#include <stdlib.h>

#include "tasks.h"

GtkWidget *task_box = NULL;
GtkWidget *search_bar = NULL;

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

gboolean search_filter(GtkListBoxRow *row, gpointer data) {
    const char *query = gtk_entry_get_text(GTK_ENTRY(search_bar));

    if (query) {
        GtkWidget *check = gtk_bin_get_child(GTK_BIN(row));
        GtkWidget *label = gtk_bin_get_child(GTK_BIN(check));
        const char *text = gtk_label_get_text(GTK_LABEL(label));

        if (!strstr(text, query)) {
            return FALSE;
        }
    }

    return TRUE;
}

void search_changed(GtkSearchEntry *search_bar) {
    gtk_list_box_invalidate_filter(GTK_LIST_BOX(task_box));
}

void add_task_clicked(GtkButton *add_task_button) {
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(add_task_button));
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL;
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Add task",
            GTK_WINDOW(window),
            flags,
            "_Add task",
            GTK_RESPONSE_ACCEPT,
            "_Cancel",
            GTK_RESPONSE_CANCEL,
            NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_set_spacing(GTK_BOX(content_area), 6);
    gtk_widget_set_margin_top(content_area, 12);
    gtk_widget_set_margin_bottom(content_area, 12);
    gtk_widget_set_margin_start(content_area, 12);
    gtk_widget_set_margin_end(content_area, 12);

    GtkWidget *desc_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(desc_entry), "Description");
    gtk_container_add(GTK_CONTAINER(content_area), desc_entry);

    gtk_widget_show_all(dialog);

    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        const char *desc = gtk_entry_get_text(GTK_ENTRY(desc_entry));

        Task *task = add_task();
        set_task_description(task, desc);

        GtkWidget *check = create_task_element(*task);
        g_signal_connect(check, "toggled", G_CALLBACK(task_toggled), (void*)task);
        gtk_container_add(GTK_CONTAINER(task_box), check);

        gtk_widget_show_all(task_box);
    }

    gtk_widget_destroy(dialog);
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

    search_bar = gtk_search_entry_new();
    g_signal_connect(search_bar, "search-changed", G_CALLBACK(search_changed), NULL);
    gtk_box_pack_start(GTK_BOX(box), search_bar, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    task_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(task_box), GTK_SELECTION_NONE);
    gtk_list_box_set_filter_func(GTK_LIST_BOX(task_box), search_filter, NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), task_box);

    for (unsigned int i = 0; i < g_num_tasks; i++) {
        GtkWidget *check = create_task_element(task_list[i]);
        g_signal_connect(check, "toggled", G_CALLBACK(task_toggled), (void*)(task_list + i));
        gtk_container_add(GTK_CONTAINER(task_box), check);
    }

    GtkWidget *add_task_button = gtk_button_new_with_label("Add task");
    g_signal_connect(add_task_button, "clicked", G_CALLBACK(add_task_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(box), add_task_button, FALSE, FALSE, 0);

    // Show the window and all its children
    gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char *argv[]) {
    init_task_list();

    for (unsigned int i = 0; i < 1000; i++) {
        Task *task = add_task();
        task->priority = 0;

        char desc[10];
        snprintf(desc, 10, "task %u", i);
        set_task_description(task, desc);
    }

    // Create GtkApplication
    GtkApplication *app = gtk_application_new("xyz.fossible.sisyphus", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(build_ui), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}

