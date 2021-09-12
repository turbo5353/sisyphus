#include <gtk/gtk.h> 

static void build_ui(GtkApplication *app) {
    // Create window
    GtkWidget *window = gtk_application_window_new(app);

    // Show the window and all its children
    gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char *argv[]) {
    // Create GtkApplication
    GtkApplication *app = gtk_application_new("xyz.fossible.sisyphus", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(build_ui), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}

