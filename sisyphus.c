#include <gtk/gtk.h> 
#include <stdio.h>
#include <stdlib.h>

#include "tasks.h"

GtkListStore *task_store = NULL;
GtkTreeModelFilter *task_filter = NULL;
GtkWidget *search_bar = NULL;

enum {
    COLUMN_CHECKED = 0,
    COLUMN_DESC,
    N_COLUMNS
};

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

void task_toggled(GtkCellRendererToggle *toggle, gchar *path_str, gpointer data) {
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    GtkTreePath *child_path = gtk_tree_model_filter_convert_path_to_child_path(task_filter, path);

    GtkTreeIter iter;
    gtk_tree_model_get_iter(GTK_TREE_MODEL(task_store), &iter, child_path);

    gboolean active = gtk_cell_renderer_toggle_get_active(toggle);

    GValue check = G_VALUE_INIT;
    g_value_init(&check, G_TYPE_BOOLEAN);
    g_value_set_boolean(&check, !active);
    gtk_list_store_set_value(task_store, &iter, COLUMN_CHECKED, &check);
    g_value_unset(&check);

    int index = gtk_tree_path_get_indices(child_path)[0];
    Task *task = &task_list[index];
    task->checked = !active;

    gtk_tree_path_free(path);
    gtk_tree_path_free(child_path);
}

gboolean search_filter(GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    const char *query = gtk_entry_get_text(GTK_ENTRY(search_bar));
    gboolean visible = TRUE;

    if (query) {
        char *str;
        gtk_tree_model_get(model, iter, COLUMN_DESC, &str, -1);

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

    GtkWidget *priority_combo_box = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(priority_combo_box), "");

    for (char pri = 'A'; pri <= 'Z'; pri++) {
        char str[] = " ";
        str[0] = pri;
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(priority_combo_box), str);
    }

    gtk_container_add(GTK_CONTAINER(content_area), priority_combo_box);

    gtk_widget_show_all(dialog);

    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        const char *desc = gtk_entry_get_text(GTK_ENTRY(desc_entry));
        char *pri_str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(priority_combo_box));

        char pri = 0;
        if (pri_str) {
            if (pri_str[0] >= 'A') {
                // 'A' - 64 = 1
                pri = pri_str[0] - 64;
            }
        }

        g_free(pri_str);

        Task *task = add_task();
        task->priority = pri;
        set_task_description(task, desc);

        GtkTreeIter iter;
        gtk_list_store_append(task_store, &iter);

        gtk_list_store_set(task_store, &iter,
                COLUMN_CHECKED, task->checked,
                COLUMN_DESC, get_task_display_string(task),
                -1);
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

    task_store = gtk_list_store_new(
            N_COLUMNS,
            G_TYPE_BOOLEAN,
            G_TYPE_STRING);

    GtkTreeIter iter;

    for (unsigned int i = 0; i < g_num_tasks; i++) {
        gtk_list_store_append(task_store, &iter);
        gtk_list_store_set(task_store, &iter,
                COLUMN_CHECKED, task_list[i].checked,
                COLUMN_DESC, get_task_display_string(&task_list[i]),
                -1);
    }

    task_filter = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new(GTK_TREE_MODEL(task_store), NULL));
    gtk_tree_model_filter_set_visible_func(task_filter, (GtkTreeModelFilterVisibleFunc)search_filter, NULL, NULL);

    GtkWidget *task_view = gtk_tree_view_new();
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
            "Description",
            renderer,
            "markup", COLUMN_DESC,
            NULL);

    gtk_tree_view_set_model(GTK_TREE_VIEW(task_view), GTK_TREE_MODEL(task_filter));
    gtk_container_add(GTK_CONTAINER(scroll), task_view);

    GtkWidget *add_task_button = gtk_button_new_with_label("Add task");
    g_signal_connect(add_task_button, "clicked", G_CALLBACK(add_task_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(box), add_task_button, FALSE, FALSE, 0);

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

