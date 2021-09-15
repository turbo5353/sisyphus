#include <gtk/gtk.h> 
#include <stdio.h>
#include <stdlib.h>

#include "tasks.h"

GtkListStore *task_store = NULL;
GtkTreeModelFilter *task_filter = NULL;
GtkWidget *search_bar = NULL;
GtkWidget *remove_task_button = NULL;
GtkTreeSelection *task_selection = NULL;

enum {
    COLUMN_CHECKED = 0,
    COLUMN_PRIORITY,
    COLUMN_DESCRIPTION,
    N_COLUMNS
};

void task_toggled(GtkCellRendererToggle *toggle, gchar *path_str, gpointer data) {
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    GtkTreePath *child_path = gtk_tree_model_filter_convert_path_to_child_path(task_filter, path);

    GtkTreeIter iter;
    gtk_tree_model_get_iter(GTK_TREE_MODEL(task_store), &iter, child_path);

    gboolean active = gtk_cell_renderer_toggle_get_active(toggle);

    int index = gtk_tree_path_get_indices(child_path)[0];
    Task *task = &task_list[index];
    task->checked = !active;
    task->priority = 0;

    GValue check = G_VALUE_INIT;
    g_value_init(&check, G_TYPE_BOOLEAN);
    g_value_set_boolean(&check, !active);
    gtk_list_store_set_value(task_store, &iter, COLUMN_CHECKED, &check);
    g_value_unset(&check);

    char *pri_str = get_task_priority_string(task);

    GValue priority = G_VALUE_INIT;
    g_value_init(&priority, G_TYPE_STRING);
    g_value_set_string(&priority, pri_str);
    gtk_list_store_set_value(task_store, &iter, COLUMN_PRIORITY, &priority);
    g_value_unset(&priority);

    free(pri_str);

    gtk_tree_path_free(path);
    gtk_tree_path_free(child_path);
}

gboolean search_filter(GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    const char *query = gtk_entry_get_text(GTK_ENTRY(search_bar));
    gboolean visible = TRUE;

    if (query) {
        char *str;
        gtk_tree_model_get(model, iter, COLUMN_DESCRIPTION, &str, -1);

        if (str) {
            if (!strstr(str, query)) {
                visible = FALSE;
            }

            g_free(str);
        }
    }

    return visible;
}

void search_changed(GtkSearchEntry *search_bar) {
    gtk_tree_model_filter_refilter(task_filter);
}

void show_edit_task_dialog(GtkWidget *window, GtkTreePath *path) {
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL;
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
            path ? "Edit task" : "Add task",
            GTK_WINDOW(window),
            flags,
            path ? "_Save" : "_Add",
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

    GtkWidget *priority_combo_box = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(priority_combo_box), "-");

    for (char pri = 'A'; pri <= 'Z'; pri++) {
        char str[] = " ";
        str[0] = pri;
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(priority_combo_box), str);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(priority_combo_box), 0);

    gtk_container_add(GTK_CONTAINER(content_area), priority_combo_box);

    GtkWidget *creation_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(creation_entry), "Creation date");
    gtk_container_add(GTK_CONTAINER(content_area), creation_entry);

    // Set values if an existing task should be edited
    if (path) {
        Task *task = &task_list[gtk_tree_path_get_indices(path)[0]];
        gtk_entry_set_text(GTK_ENTRY(desc_entry), task->description);
        gtk_combo_box_set_active(GTK_COMBO_BOX(priority_combo_box), task->priority);
        gtk_entry_set_text(GTK_ENTRY(creation_entry), task->creation_date);
    }

    gtk_widget_show_all(dialog);

    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        char *pri_str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(priority_combo_box));

        char pri = 0;
        if (pri_str) {
            if (pri_str[0] >= 'A' && pri_str[0] <= 'Z') {
                // 'A' - 64 = 1
                pri = pri_str[0] - 64;
            }
        }

        g_free(pri_str);

        Task *task = NULL;
        GtkTreeIter iter;
        if (path) {
            task = &task_list[gtk_tree_path_get_indices(path)[0]];
            gtk_tree_model_get_iter(GTK_TREE_MODEL(task_store), &iter, path);
        }
        else {
            task = add_task();
            gtk_list_store_append(task_store, &iter);
        }

        task->priority = pri;
        const char *creation_date = gtk_entry_get_text(GTK_ENTRY(creation_entry));
        strcpy(task->creation_date, creation_date);
        const char *desc = gtk_entry_get_text(GTK_ENTRY(desc_entry));
        set_task_description(task, desc);

        char *pri_display_str = get_task_priority_string(task);
        char *display_str = get_task_display_string(task);

        gtk_list_store_set(task_store, &iter,
                COLUMN_CHECKED, task->checked,
                COLUMN_PRIORITY, pri_display_str,
                COLUMN_DESCRIPTION, display_str,
                -1);

        free(pri_display_str);
        free(display_str);
    }

    gtk_widget_destroy(dialog);
}

void task_row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data) {
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(view));
    GtkTreePath *child_path = gtk_tree_model_filter_convert_path_to_child_path(task_filter, path);
    show_edit_task_dialog(window, child_path);
    gtk_tree_path_free(child_path);
}

void add_task_clicked(GtkButton *add_task_button) {
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(add_task_button));
    show_edit_task_dialog(window, NULL);
}

void remove_task_clicked(GtkButton *remove_task_button) {
    if (task_selection) {
        GtkTreeIter iter;
        if (gtk_tree_selection_get_selected(task_selection, NULL, &iter)) {
            GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(task_filter), &iter);
            GtkTreePath *child_path = gtk_tree_model_filter_convert_path_to_child_path(task_filter, path);

            gtk_tree_model_get_iter(GTK_TREE_MODEL(task_store), &iter, child_path);
            gtk_list_store_remove(task_store, &iter);
            remove_task(gtk_tree_path_get_indices(child_path)[0]);

            gtk_tree_path_free(path);
            gtk_tree_path_free(child_path);
        }
    }
}

void task_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
    if (remove_task_button) {
        gtk_widget_set_sensitive(remove_task_button, gtk_tree_selection_get_selected(selection, NULL, NULL));
    }
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

    task_store = gtk_list_store_new(
            N_COLUMNS,
            G_TYPE_BOOLEAN,
            G_TYPE_STRING,
            G_TYPE_STRING);

    GtkTreeIter iter;

    for (unsigned int i = 0; i < g_num_tasks; i++) {
        gtk_list_store_append(task_store, &iter);

        char *pri_display_str = get_task_priority_string(&task_list[i]);
        char *display_str = get_task_display_string(&task_list[i]);

        gtk_list_store_set(task_store, &iter,
                COLUMN_CHECKED, task_list[i].checked,
                COLUMN_PRIORITY, pri_display_str,
                COLUMN_DESCRIPTION, display_str,
                -1);

        free(pri_display_str);
        free(display_str);
    }

    task_filter = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new(GTK_TREE_MODEL(task_store), NULL));
    gtk_tree_model_filter_set_visible_func(task_filter, (GtkTreeModelFilterVisibleFunc)search_filter, NULL, NULL);

    GtkWidget *task_view = gtk_tree_view_new();
    g_signal_connect(task_view, "row-activated", G_CALLBACK(task_row_activated), NULL);

    task_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(task_view));
    g_signal_connect(task_selection, "changed", G_CALLBACK(task_selection_changed), NULL);

    GtkCellRenderer *renderer;

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled", G_CALLBACK(task_toggled), NULL);
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(task_view),
            -1,
            "Checked",
            renderer,
            "active", COLUMN_CHECKED,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(task_view),
            -1,
            "Priority",
            renderer,
            "markup", COLUMN_PRIORITY,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(task_view),
            -1,
            "Description",
            renderer,
            "markup", COLUMN_DESCRIPTION,
            NULL);

    gtk_tree_view_set_model(GTK_TREE_VIEW(task_view), GTK_TREE_MODEL(task_filter));
    gtk_container_add(GTK_CONTAINER(scroll), task_view);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(box), button_box, FALSE, FALSE, 0);

    GtkWidget *add_task_button = gtk_button_new_with_label("Add task");
    g_signal_connect(add_task_button, "clicked", G_CALLBACK(add_task_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), add_task_button, TRUE, TRUE, 0);

    remove_task_button = gtk_button_new_with_label("Remove task");
    gtk_widget_set_sensitive(remove_task_button, FALSE);
    g_signal_connect(remove_task_button, "clicked", G_CALLBACK(remove_task_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), remove_task_button, TRUE, TRUE, 0);

    // Show the window and all its children
    gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char *argv[]) {
    // Make printf print immediately
    setbuf(stdout, NULL);
    init_task_list();

    for (unsigned int i = 0; i < 5; i++) {
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

