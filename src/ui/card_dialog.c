#include "card_dialog.h"
#include "../db.h"

typedef struct {
    GtkWidget *dialog;
    GtkWidget *title_entry;
    GtkWidget *date_btn;
    GtkWidget *calendar;
    GtkWidget *desc_view;
    Card *card;
    GtkWidget *parent_widget;
} DialogState;

static void on_date_selected(GtkCalendar *calendar, gpointer data) {
    DialogState *state = (DialogState *)data;
    GDateTime *dt = gtk_calendar_get_date(calendar);
    if (dt) {
        char *date_str = g_date_time_format(dt, "%d %b %Y");
        gtk_menu_button_set_label(GTK_MENU_BUTTON(state->date_btn), date_str);
        g_free(date_str);
        g_date_time_unref(dt);
    }
}

static void on_close_clicked(GtkWidget *btn, gpointer data) {
    DialogState *state = (DialogState *)data;
    gtk_window_destroy(GTK_WINDOW(state->dialog));
    g_free(state);
}

static void on_save_clicked(GtkWidget *btn, gpointer data) {
    DialogState *state = (DialogState *)data;
    
    const char *new_title = gtk_editable_get_text(GTK_EDITABLE(state->title_entry));
    const char *new_due = gtk_menu_button_get_label(GTK_MENU_BUTTON(state->date_btn));
    if (strcmp(new_due, "Select Date") == 0) new_due = "";
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->desc_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *desc_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    // Update DB
    strncpy(state->card->name, new_title, MAX_NAME_LEN - 1);
    strncpy(state->card->description, desc_text, MAX_DESC_LEN - 1);
    strncpy(state->card->due_date, new_due, 31);
    db_update_card(state->card);
    
    // Update UI button label (for a complex card, we'd rebuild the content, but for now we'll just set it if it's a simple label button)
    // Wait, the card button might have a custom child soon to show the due date. Let's call a rebuild function or simply re-create it?
    // The easiest way is to let ui_card_create return the updated widget and swap it, or modify the box.
    // For now we will update the label (we will add due date rendering in card.c).
    // Actually, I can just destroy the parent_widget and replace it? No, parent_widget is the GtkButton.
    // In card.c we will rebuild its child. Let's create an 'ui_card_update(GtkWidget *card_btn, Card *card)' in card.c
    // Let's declare it here:
    extern void ui_card_update_view(GtkWidget *card_btn, Card *card);
    ui_card_update_view(state->parent_widget, state->card);
    
    g_free(desc_text);
    gtk_window_destroy(GTK_WINDOW(state->dialog));
    g_free(state);
}

static void on_delete_clicked(GtkWidget *btn, gpointer data) {
    DialogState *state = (DialogState *)data;
    
    // Delete from database
    db_delete_card(state->card->id);
    
    // Remove the card widget from its parent
    GtkWidget *parent_box = gtk_widget_get_parent(state->parent_widget);
    if (parent_box) {
        gtk_box_remove(GTK_BOX(parent_box), state->parent_widget);
    }
    
    gtk_window_destroy(GTK_WINDOW(state->dialog));
    g_free(state);
}

void ui_card_dialog_show(Card *card, GtkWidget *parent_widget) {
    GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_root(parent_widget));
    DialogState *state = g_new0(DialogState, 1);
    state->card = card;
    state->parent_widget = parent_widget;

    state->dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(state->dialog), "Edit Card");
    gtk_window_set_default_size(GTK_WINDOW(state->dialog), 500, 650);
    gtk_window_set_transient_for(GTK_WINDOW(state->dialog), parent_window);
    gtk_window_set_modal(GTK_WINDOW(state->dialog), TRUE);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(content, 24);
    gtk_widget_set_margin_end(content, 24);
    gtk_widget_set_margin_top(content, 24);
    gtk_widget_set_margin_bottom(content, 24);
    gtk_window_set_child(GTK_WINDOW(state->dialog), content);

    GtkWidget *title_label = gtk_label_new("Title");
    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(content), title_label);

    state->title_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(state->title_entry), card->name);
    gtk_box_append(GTK_BOX(content), state->title_entry);
    
    GtkWidget *due_label = gtk_label_new("Due Date");
    gtk_widget_set_halign(due_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(content), due_label);

    state->date_btn = gtk_menu_button_new();
    if (card->due_date && strlen(card->due_date) > 0) {
        gtk_menu_button_set_label(GTK_MENU_BUTTON(state->date_btn), card->due_date);
    } else {
        gtk_menu_button_set_label(GTK_MENU_BUTTON(state->date_btn), "Select Date");
    }
    
    GtkWidget *popover = gtk_popover_new();
    state->calendar = gtk_calendar_new();
    gtk_popover_set_child(GTK_POPOVER(popover), state->calendar);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(state->date_btn), popover);
    
    g_signal_connect(state->calendar, "day-selected", G_CALLBACK(on_date_selected), state);
    
    gtk_box_append(GTK_BOX(content), state->date_btn);

    GtkWidget *desc_label = gtk_label_new("Description");
    gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(content), desc_label);

    GtkWidget *scrolled_desc = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scrolled_desc, -1, 150);
    gtk_box_append(GTK_BOX(content), scrolled_desc);

    state->desc_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(state->desc_view), GTK_WRAP_WORD_CHAR);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->desc_view));
    gtk_text_buffer_set_text(buffer, card->description, -1);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_desc), state->desc_view);

    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(content), spacer);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(content), button_box);

    GtkWidget *btn_delete = gtk_button_new_with_label("Delete");
    gtk_widget_add_css_class(btn_delete, "destructive-action");
    gtk_widget_set_halign(btn_delete, GTK_ALIGN_START);
    
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_hexpand(right_box, TRUE);
    gtk_widget_set_halign(right_box, GTK_ALIGN_END);

    GtkWidget *btn_cancel = gtk_button_new_with_label("Cancel");
    GtkWidget *btn_save = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(btn_save, "suggested-action");
    
    g_signal_connect(btn_delete, "clicked", G_CALLBACK(on_delete_clicked), state);
    g_signal_connect(btn_cancel, "clicked", G_CALLBACK(on_close_clicked), state);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_clicked), state);

    gtk_box_append(GTK_BOX(button_box), btn_delete);
    gtk_box_append(GTK_BOX(button_box), right_box);
    gtk_box_append(GTK_BOX(right_box), btn_cancel);
    gtk_box_append(GTK_BOX(right_box), btn_save);

    gtk_window_present(GTK_WINDOW(state->dialog));
}
