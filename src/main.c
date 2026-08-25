#include <gtk/gtk.h>
#include "db.h"
#include "ui/window.h"

static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    
    // Load custom CSS for trello-like styles
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { background: #1d2125; }\n"
        ".board-page { background: transparent; }\n"
        ".top-bar { background: rgba(0,0,0,0.4); padding: 8px; border-radius: 4px; }\n"
        ".list-title-dark { font-weight: bold; font-size: 11pt; color: #b6c2cf; }\n"
        ".list-title-entry { font-weight: bold; font-size: 11pt; color: #b6c2cf; background: transparent; border: none; box-shadow: none; padding: 2px; }\n"
        ".list-title-entry:focus { background: #282e33; border-radius: 4px; }\n"
        ".board-title-entry { font-weight: bold; font-size: 14pt; background: transparent; color: white; border: none; box-shadow: none; }\n"
        ".board-title-entry:focus { background: rgba(255,255,255,0.2); }\n"
        ".board-card { padding: 0; background-image: none; background: #333c43; color: #ffffff; border-radius: 8px; border: none; box-shadow: 0 2px 4px rgba(0,0,0,0.5); }\n"
        ".board-card label { color: #ffffff; font-weight: bold; }\n"
        ".board-card:hover { background-image: none; background: #404b54; }\n"
        ".board-card-create { background: transparent; color: #b6c2cf; border-radius: 8px; border: 1px dashed #b6c2cf; }\n"
        ".board-card-create:hover { background: rgba(255,255,255,0.05); }\n"
        ".view-dark { background: #101204; border-radius: 12px; }\n"
        "button.card-dark { background-image: none; background: #4a5259; border-radius: 8px; color: #b6c2cf; padding: 8px 12px; margin-bottom: 4px; border: none; font-size: 11pt; box-shadow: none; }\n"
        "button.card-dark:hover { background-image: none; background: #5c666f; }\n"
        ".card-due-date { background: rgba(255, 255, 255, 0.1); color: #b6c2cf; padding: 2px 4px; border-radius: 4px; font-size: 9pt; margin-top: 4px; }\n"
        ".placeholder-card { background: rgba(255, 255, 255, 0.05); border-radius: 8px; margin-bottom: 4px; }\n"
        ".placeholder-list { background: rgba(255, 255, 255, 0.05); border-radius: 12px; }\n"
        "label.list-title-dark { color: #b6c2cf; font-weight: bold; font-size: 11pt; padding: 6px; }\n"
        "label.list-count-dark { color: #b6c2cf; font-size: 10pt; padding: 6px; }\n"
        "button.delete-btn-dark { background: transparent; color: #b6c2cf; font-weight: bold; font-size: 14pt; padding: 0 8px; border: none; border-radius: 4px; }\n"
        "button.delete-btn-dark:hover { background: #282e33; }\n"
        "button.add-card-btn-dark { background: transparent; color: #b6c2cf; border: none; border-radius: 6px; padding: 8px; font-weight: bold; font-size: 11pt; }\n"
        "button.add-card-btn-dark:hover { background: #282e33; }\n"
        "button.add-list-btn { background-image: none; background: rgba(0, 0, 0, 0.35); color: #ffffff; border: none; border-radius: 8px; padding: 12px; font-weight: bold; font-size: 11pt; text-shadow: 0 1px 2px rgba(0,0,0,0.8); }\n"
        "button.add-list-btn:hover { background-image: none; background: rgba(0, 0, 0, 0.45); }\n"
        ".board-card-label-box { background: #282e33; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px; }\n"
        ".board-card-image { border-top-left-radius: 8px; border-top-right-radius: 8px; }\n"
        ".list-container { background-image: none; background: #101204; border-radius: 8px; }\n"
        "scrolledwindow.list-scroll undershoot, scrolledwindow.list-scroll overshoot { background: none; }\n"
        "scrolledwindow.list-scroll scrollbar { min-width: 0px; min-height: 0px; opacity: 0; }\n"
        ".inline-list-form { background: #101204; border-radius: 8px; padding: 8px; }\n"
        "entry.card-dark { background-image: none; background-color: #22272b; color: white; min-height: 24px; padding: 4px 6px; border-radius: 6px; margin: 0 4px; }\n"
        "box:drop(active) { box-shadow: none; border: none; }\n"
        ".list-container.drag-hover { border: 2px solid #77ff77; box-shadow: 0 0 6px #77ff77; }");
        
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);

    ui_window_init(app);
}

int main(int argc, char **argv) {
    if (!db_init("task-board.db")) {
        return 1;
    }

    GtkApplication *app = gtk_application_new("com.example.taskboard", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    g_object_unref(app);
    db_close();
    return status;
}
