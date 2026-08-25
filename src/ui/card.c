#include "card.h"
#include "card_dialog.h"
#include <stdlib.h>

static void on_card_clicked(GtkButton *btn, gpointer user_data) {
    Card *card = (Card *)user_data;
    ui_card_dialog_show(card, GTK_WIDGET(btn));
}

// Drag callbacks
GtkWidget *global_dragged_card = NULL;

static void on_drag_begin(GtkDragSource *source, GdkDrag *drag, gpointer data) {
    (void)source; (void)drag;
    if (global_dragged_card) g_object_remove_weak_pointer(G_OBJECT(global_dragged_card), (gpointer *)&global_dragged_card);
    global_dragged_card = GTK_WIDGET(data);
    g_object_add_weak_pointer(G_OBJECT(global_dragged_card), (gpointer *)&global_dragged_card);
    gtk_widget_set_opacity(global_dragged_card, 0.4);
    
    GdkPaintable *paintable = gtk_widget_paintable_new(global_dragged_card);
    gtk_drag_source_set_icon(source, paintable, 0, 0);
    g_object_unref(paintable);
}

static void on_drag_end(GtkDragSource *source, GdkDrag *drag, gboolean delete_data, gpointer data) {
    (void)source; (void)drag; (void)delete_data; (void)data;
    if (global_dragged_card) {
        gtk_widget_set_opacity(global_dragged_card, 1.0);
        g_object_remove_weak_pointer(G_OBJECT(global_dragged_card), (gpointer *)&global_dragged_card);
    }
    global_dragged_card = NULL;
}

void ui_card_update_view(GtkWidget *card_btn, Card *card) {
    // We expect card_btn to be a button containing a VBox
    GtkWidget *vbox = gtk_button_get_child(GTK_BUTTON(card_btn));
    if (!vbox) return;
    
    // Clear old children
    GtkWidget *child = gtk_widget_get_first_child(vbox);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(vbox), child);
        child = next;
    }
    
    // Add title
    GtkWidget *title = gtk_label_new(card->name);
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(title), TRUE);
    gtk_box_append(GTK_BOX(vbox), title);
    
    // Add due date if exists
    if (strlen(card->due_date) > 0) {
        GtkWidget *due = gtk_label_new(card->due_date);
        gtk_widget_add_css_class(due, "card-due-date");
        gtk_widget_set_halign(due, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(vbox), due);
    }
}

GtkWidget* ui_card_create(Card *card) {
    GtkWidget *card_btn = gtk_button_new();
    gtk_widget_set_size_request(card_btn, -1, 60);
    gtk_widget_add_css_class(card_btn, "card-dark");
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_button_set_child(GTK_BUTTON(card_btn), vbox);
    
    ui_card_update_view(card_btn, card);
    
    // Allocate memory for the card struct to pass to the dialog
    Card *card_copy = g_new0(Card, 1);
    *card_copy = *card;
    
    g_signal_connect_data(card_btn, "clicked", G_CALLBACK(on_card_clicked), card_copy, (GClosureNotify)g_free, 0);

    // Setup drag source
    GtkDragSource *drag_source = gtk_drag_source_new();
    gtk_drag_source_set_actions(drag_source, GDK_ACTION_MOVE);

    // Create a payload. The string format can be "card_id"
    char payload_str[32];
    snprintf(payload_str, sizeof(payload_str), "%d", card->id);
    GValue val = G_VALUE_INIT;
    g_value_init(&val, G_TYPE_STRING);
    g_value_set_string(&val, payload_str);
    GdkContentProvider *provider = gdk_content_provider_new_for_value(&val);
    gtk_drag_source_set_content(drag_source, provider);
    g_object_unref(provider);
    g_value_unset(&val);

    g_signal_connect(drag_source, "drag-begin", G_CALLBACK(on_drag_begin), card_btn);
    g_signal_connect(drag_source, "drag-end", G_CALLBACK(on_drag_end), NULL);
    
    gtk_widget_add_controller(card_btn, GTK_EVENT_CONTROLLER(drag_source));
    
    // Store card_id on the widget
    g_object_set_data(G_OBJECT(card_btn), "card_id", GINT_TO_POINTER(card->id));

    return card_btn;
}
