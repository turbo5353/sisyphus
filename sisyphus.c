#include <gtk/gtk.h> 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "tasks.h"

size_t filename_size = 0;
char *filename = NULL;
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

void set_filename(const char *str) {
    size_t len = strlen(str);
    if (len >= filename_size) {
        filename = (char*) realloc(filename, (len + 1) * sizeof(char));
        filename_size = len + 1;
    }

    strcpy(filename, str);
}

void task_toggled(GtkCellRendererToggle *toggle, gchar *path_str, gpointer data) {
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    GtkTreePath *child_path = gtk_tree_model_filter_convert_path_to_child_path(task_filter, path);

    GtkTreeIter iter;
    gtk_tree_model_get_iter(GTK_TREE_MODEL(task_store), &iter, child_path);

    gboolean new_active = !gtk_cell_renderer_toggle_get_active(toggle);

    int index = gtk_tree_path_get_indices(child_path)[0];
    Task *task = &task_list[index];
    task->checked = new_active;

    if (new_active) {
        task->priority = 0;
        set_task_completion_time_now(task);
        char *pri_str = get_task_priority_string(task);

        GValue priority = G_VALUE_INIT;
        g_value_init(&priority, G_TYPE_STRING);
        g_value_set_string(&priority, pri_str);
        gtk_list_store_set_value(task_store, &iter, COLUMN_PRIORITY, &priority);
        g_value_unset(&priority);

        free(pri_str);
    }

    GValue check = G_VALUE_INIT;
    g_value_init(&check, G_TYPE_BOOLEAN);
    g_value_set_boolean(&check, new_active);
    gtk_list_store_set_value(task_store, &iter, COLUMN_CHECKED, &check);
    g_value_unset(&check);

    gtk_tree_path_free(path);
    gtk_tree_path_free(child_path);

    write_file(filename);
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

    time_t raw_time = time(NULL);
    struct tm *time_info = localtime(&raw_time);

    // Creation date
    GtkWidget *creation_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_add(GTK_CONTAINER(content_area), creation_box);

    GtkWidget *creation_day = gtk_spin_button_new_with_range(1, 31, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(creation_day), time_info->tm_mday);
    gtk_box_pack_start(GTK_BOX(creation_box), creation_day, TRUE, TRUE, 0);

    GtkWidget *creation_month = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(creation_box), creation_month, TRUE, TRUE, 0);

    GtkWidget *creation_year = gtk_spin_button_new_with_range(1000, 9999, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(creation_year), time_info->tm_year + 1900);
    gtk_box_pack_start(GTK_BOX(creation_box), creation_year, TRUE, TRUE, 0);

    // Completion date
    GtkWidget *completion_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_sensitive(completion_box, FALSE);
    gtk_container_add(GTK_CONTAINER(content_area), completion_box);

    GtkWidget *completion_day = gtk_spin_button_new_with_range(1, 31, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(completion_day), time_info->tm_mday);
    gtk_box_pack_start(GTK_BOX(completion_box), completion_day, TRUE, TRUE, 0);

    GtkWidget *completion_month = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(completion_box), completion_month, TRUE, TRUE, 0);

    GtkWidget *completion_year = gtk_spin_button_new_with_range(1000, 9999, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(completion_year), time_info->tm_year + 1900);
    gtk_box_pack_start(GTK_BOX(completion_box), completion_year, TRUE, TRUE, 0);

    // Month names
    const char *months[] = {
        "January",
        "February",
        "March",
        "April",
        "May",
        "June",
        "July",
        "August",
        "September",
        "October",
        "November",
        "December"
    };

    // Fill month combo boxes
    for (unsigned int i = 0; i < 12; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(creation_month), months[i]);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(completion_month), months[i]);
    }

    // Set current month as the active month of both combo boxes
    gtk_combo_box_set_active(GTK_COMBO_BOX(creation_month), time_info->tm_mon);
    gtk_combo_box_set_active(GTK_COMBO_BOX(completion_month), time_info->tm_mon);

    // Set values if an existing task should be edited
    if (path) {
        Task *task = &task_list[gtk_tree_path_get_indices(path)[0]];
        gtk_entry_set_text(GTK_ENTRY(desc_entry), task->description);
        gtk_combo_box_set_active(GTK_COMBO_BOX(priority_combo_box), task->priority);

        gtk_spin_button_set_value(GTK_SPIN_BUTTON(creation_day), task->creation_day);
        gtk_combo_box_set_active(GTK_COMBO_BOX(creation_month), task->creation_month);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(creation_year), task->creation_year);

        if (task->checked) {
            gtk_widget_set_sensitive(completion_box, TRUE);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(completion_day), task->completion_day);
            gtk_combo_box_set_active(GTK_COMBO_BOX(completion_month), task->completion_month);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(completion_year), task->completion_year);
        }
    }

    gtk_widget_show_all(dialog);

    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
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

        // Set task values to the values from the dialog box
        task->priority = gtk_combo_box_get_active(GTK_COMBO_BOX(priority_combo_box));

        task->creation_day = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(creation_day));
        task->creation_month = gtk_combo_box_get_active(GTK_COMBO_BOX(creation_month));
        task->creation_year = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(creation_year));

        task->completion_day = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(completion_day));
        task->completion_month = gtk_combo_box_get_active(GTK_COMBO_BOX(completion_month));
        task->completion_year = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(completion_year));

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

        write_file(filename);
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

            write_file(filename);
        }
    }
}

void task_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
    if (remove_task_button) {
        gtk_widget_set_sensitive(remove_task_button, gtk_tree_selection_get_selected(selection, NULL, NULL));
    }
}

void load_task_store() {
    gtk_list_store_clear(task_store);

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
}

void open_file_clicked(GtkWidget *widget, gpointer window) {
    GtkFileChooserNative *native = gtk_file_chooser_native_new(
            "Open File",
            window,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Open",
            "_Cancel");

    gint res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(native));

        if (!read_file(file)) {
            GtkWidget *error_dialog = gtk_message_dialog_new(
                    window,
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    "Could not open \"%s\": %s",
                    file,
                    g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        }
        else {
            set_filename(file);
            load_task_store();
        }

        g_free(file);
    }

    g_object_unref(native);
}

void save_as_clicked(GtkWidget *widget, gpointer window) {
    GtkFileChooserNative *native = gtk_file_chooser_native_new(
            "Save as",
            window,
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Save",
            "_Cancel");

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(native), TRUE);

    if (!filename || strlen(filename) == 0) {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(native), "todo.txt");
    }
    else {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(native), filename);
    }

    gint res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(native));
        set_filename(file);
        g_free(file);

        write_file(filename);
    }
}

void about_clicked(GtkWidget *widget, gpointer window) {
    const char *authors[] = { "Tobias Wienkoop", NULL };
    gtk_show_about_dialog(
            window,
            "authors", authors,
            "comments", "Simple todo.txt editor",
            "copyright", "Copyright \u00A9 2021 Tobias Wienkoop",
            "license-type", GTK_LICENSE_GPL_3_0,
            NULL);
}

void build_ui(GtkApplication *app) {
    // Create window
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), box);

    // Create menu
    GtkWidget *menu_bar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(box), menu_bar, FALSE, FALSE, 0);

    // File menu
    GtkWidget *file_menu_item = gtk_menu_item_new_with_mnemonic("_File");
    GtkWidget *file_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);

    GtkWidget *file_open_item = gtk_menu_item_new_with_label("Open");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), file_open_item);
    g_signal_connect(file_open_item, "activate", G_CALLBACK(open_file_clicked), window);

    GtkWidget *file_save_item = gtk_menu_item_new_with_label("Save as");
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), file_save_item);
    g_signal_connect(file_save_item, "activate", G_CALLBACK(save_as_clicked), window);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_menu_item);

    // Help menu
    GtkWidget *help_menu_item = gtk_menu_item_new_with_mnemonic("_Help");
    GtkWidget *help_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menu_item), help_menu);

    GtkWidget *help_about = gtk_menu_item_new_with_label("About");
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), help_about);
    g_signal_connect(help_about, "activate", G_CALLBACK(about_clicked), window);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), help_menu_item);

    // Create window content
    GtkWidget *margin_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(margin_box, 12);
    gtk_widget_set_margin_bottom(margin_box, 12);
    gtk_widget_set_margin_start(margin_box, 12);
    gtk_widget_set_margin_end(margin_box, 12);
    gtk_box_pack_start(GTK_BOX(box), margin_box, TRUE, TRUE, 0);

    search_bar = gtk_search_entry_new();
    g_signal_connect(search_bar, "search-changed", G_CALLBACK(search_changed), NULL);
    gtk_box_pack_start(GTK_BOX(margin_box), search_bar, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(margin_box), scroll, TRUE, TRUE, 0);

    task_store = gtk_list_store_new(
            N_COLUMNS,
            G_TYPE_BOOLEAN,
            G_TYPE_STRING,
            G_TYPE_STRING);

    load_task_store();

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
    gtk_box_pack_start(GTK_BOX(margin_box), button_box, FALSE, FALSE, 0);

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

void open_file(GtkApplication *app, GFile **files, gint n_files, gchar *hint, gpointer data) {
    if (n_files > 0) {
        char *file = g_file_get_path(files[0]);
        set_filename(file);
        g_free(file);
    }

    if (!read_file(filename)) {
        GtkWidget *error_dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                "Could not open \"%s\": %s",
                filename,
                g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
    }
    else {
        build_ui(app);
    }
}

int main(int argc, char *argv[]) {
    // Make printf print immediately
    setbuf(stdout, NULL);

    // Create GtkApplication
    GtkApplication *app = gtk_application_new("xyz.fossible.sisyphus", G_APPLICATION_HANDLES_OPEN);
    g_signal_connect(app, "activate", G_CALLBACK(build_ui), NULL);
    g_signal_connect(app, "open", G_CALLBACK(open_file), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}

