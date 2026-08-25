#include "board.h"
#include "card.h"
#include "../db.h"
#include <stdio.h>
#include <stdlib.h>

static void on_list_title_focus_leave(GtkEventControllerFocus *controller, gpointer user_data) {
    GtkWidget *col_box = GTK_WIDGET(user_data);
    GtkWidget *entry = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
    int list_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(col_box), "list_id"));
    const char *new_title = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (new_title && strlen(new_title) > 0) {
        db_update_list_name(list_id, new_title);
    }
}

static void on_list_title_changed(GtkEditable *editable, gpointer user_data) {
    GtkWidget *col_box = GTK_WIDGET(user_data);
    int list_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(col_box), "list_id"));
    const char *new_title = gtk_editable_get_text(editable);
    if (new_title && strlen(new_title) > 0) {
        db_update_list_name(list_id, new_title);
    }
}


static void on_delete_list_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    GtkWidget *col_box = GTK_WIDGET(user_data);
    int list_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(col_box), "list_id"));
    db_delete_list(list_id);
    GtkWidget *parent = gtk_widget_get_parent(col_box);
    if (parent) {
        gtk_box_remove(GTK_BOX(parent), col_box);
    }
}

static void safe_remove_entry(GtkWidget *entry, GtkWidget *box) {
    if (GTK_IS_WIDGET(entry) && gtk_widget_get_parent(entry) == box) {
        GtkEventController *ctrl = g_object_get_data(G_OBJECT(entry), "focus_ctrl");
        if (ctrl) {
            g_object_set_data(G_OBJECT(entry), "focus_ctrl", NULL);
            gtk_widget_remove_controller(entry, ctrl);
        }
        gtk_box_remove(GTK_BOX(box), entry);
    }
}

static gboolean idle_remove_entry(gpointer data) {
    GtkWidget *entry = GTK_WIDGET(data);
    if (GTK_IS_WIDGET(entry)) {
        GtkWidget *parent = gtk_widget_get_parent(entry);
        if (parent) {
            safe_remove_entry(entry, parent);
        }
    }
    return G_SOURCE_REMOVE;
}

static void on_inline_card_activate(GtkEntry *entry, gpointer user_data) {
    GtkWidget *card_list_box = GTK_WIDGET(user_data);
    if (!GTK_IS_WIDGET(card_list_box)) return;
    
    int list_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(card_list_box), "list_id"));
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
    
    if (text && strlen(text) > 0) {
        int new_id = db_create_card(list_id, text, 0); 
        if (new_id > 0) {
            Card card = {0};
            card.id = new_id;
            card.list_id = list_id;
            strncpy(card.name, text, MAX_NAME_LEN - 1);
            
            GtkWidget *card_widget = ui_card_create(&card);
            gtk_box_insert_child_after(GTK_BOX(card_list_box), card_widget, gtk_widget_get_prev_sibling(GTK_WIDGET(entry)));
        }
        gtk_editable_set_text(GTK_EDITABLE(entry), "");
    } else {
        g_idle_add(idle_remove_entry, entry);
    }
}

static void on_inline_card_focus_leave(GtkEventControllerFocus *controller, gpointer user_data) {
    (void)controller;
    GtkWidget *entry = GTK_WIDGET(user_data);
    if (!GTK_IS_WIDGET(entry)) return;
    
    GtkWidget *card_list_box = gtk_widget_get_parent(entry);
    if (!card_list_box) return;
    
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (text && strlen(text) > 0) {
        int list_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(card_list_box), "list_id"));
        int new_id = db_create_card(list_id, text, 0); 
        if (new_id > 0) {
            Card card = {0};
            card.id = new_id;
            card.list_id = list_id;
            strncpy(card.name, text, MAX_NAME_LEN - 1);
            
            GtkWidget *card_widget = ui_card_create(&card);
            gtk_box_insert_child_after(GTK_BOX(card_list_box), card_widget, gtk_widget_get_prev_sibling(GTK_WIDGET(entry)));
        }
    }
    g_idle_add(idle_remove_entry, entry);
}

static void on_add_card_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    GtkWidget *card_list_box = GTK_WIDGET(user_data);
    
    GtkWidget *entry = gtk_entry_new();
    gtk_widget_add_css_class(entry, "card-dark");
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter a title for this card...");
    
    gtk_box_append(GTK_BOX(card_list_box), entry);
    g_signal_connect(entry, "activate", G_CALLBACK(on_inline_card_activate), card_list_box);
    
    GtkEventController *focus_ctrl = gtk_event_controller_focus_new();
    g_signal_connect(focus_ctrl, "leave", G_CALLBACK(on_inline_card_focus_leave), entry);
    gtk_widget_add_controller(entry, focus_ctrl);
    g_object_set_data(G_OBJECT(entry), "focus_ctrl", focus_ctrl);
    
    gtk_widget_grab_focus(entry);
}

GtkWidget *placeholder_card = NULL;

static void create_placeholder() {
    if (!placeholder_card) {
        placeholder_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_size_request(placeholder_card, -1, 60);
        gtk_widget_add_css_class(placeholder_card, "placeholder-card");
        g_object_ref_sink(placeholder_card);
    }
}

static GdkDragAction on_drag_motion(GtkDropTarget *target, double x, double y, gpointer data) {
    (void)target; (void)x;
    GtkWidget *card_list_box = GTK_WIDGET(data);
    
        if (!global_dragged_card) return GDK_ACTION_MOVE;
    GtkWidget *col_box = GTK_WIDGET(g_object_get_data(G_OBJECT(card_list_box), "col_box"));
    if (col_box) gtk_widget_add_css_class(col_box, "drag-hover");

    create_placeholder();

    GtkWidget *parent = gtk_widget_get_parent(placeholder_card);
    if (parent != card_list_box) {
        if (parent) gtk_box_remove(GTK_BOX(parent), placeholder_card);
        gtk_box_append(GTK_BOX(card_list_box), placeholder_card);
    }
    
    GtkWidget *child = gtk_widget_get_first_child(card_list_box);
    GtkWidget *insert_before = NULL;
    
    while (child) {
        if (child != placeholder_card && child != global_dragged_card) {
            graphene_point_t p;
            if (gtk_widget_compute_point(child, card_list_box, &GRAPHENE_POINT_INIT(0, 0), &p)) {
                int height = gtk_widget_get_height(child);
                if (y < p.y + height / 2.0) {
                    insert_before = child;
                    break;
                }
            }
        }
        child = gtk_widget_get_next_sibling(child);
    }
    
    GtkWidget *new_prev = insert_before ? gtk_widget_get_prev_sibling(insert_before) : gtk_widget_get_last_child(card_list_box);
    // If the new prev is the dragged card, skip over it
    if (new_prev == global_dragged_card) new_prev = gtk_widget_get_prev_sibling(new_prev);
    // If the new prev is the placeholder itself, no need to move
    if (new_prev == placeholder_card) new_prev = gtk_widget_get_prev_sibling(placeholder_card);
    
    GtkWidget *current_prev = gtk_widget_get_prev_sibling(placeholder_card);
    if (current_prev != new_prev) {
        gtk_box_reorder_child_after(GTK_BOX(card_list_box), placeholder_card, new_prev);
    }
    return GDK_ACTION_MOVE;
}

static gboolean idle_remove_placeholder_card(gpointer data) {
    if (data) {
        GtkWidget *card_list_box = GTK_WIDGET(data);
        GtkWidget *col_box = GTK_WIDGET(g_object_get_data(G_OBJECT(card_list_box), "col_box"));
        if (col_box) gtk_widget_remove_css_class(col_box, "drag-hover");
    }
    if (placeholder_card) {
        GtkWidget *parent = gtk_widget_get_parent(placeholder_card);
        if (parent) gtk_box_remove(GTK_BOX(parent), placeholder_card);
    }
    return G_SOURCE_REMOVE;
}

static void on_drag_leave(GtkDropTarget *target, gpointer data) {
    (void)target; (void)data;
    g_idle_add(idle_remove_placeholder_card, data);
}

typedef struct {
    GtkWidget *card;
    GtkWidget *old_parent;
    GtkWidget *new_parent;
    GtkWidget *insert_after;
    int new_list_id;
} CardMoveData;

static gboolean idle_move_card(gpointer data) {
    CardMoveData *cmd = data;
    
    if (cmd->old_parent && GTK_IS_WIDGET(cmd->old_parent) && GTK_IS_WIDGET(cmd->card)) {
        gtk_box_remove(GTK_BOX(cmd->old_parent), cmd->card);
    }
    
    if (GTK_IS_WIDGET(cmd->new_parent) && GTK_IS_WIDGET(cmd->card)) {
        gtk_box_insert_child_after(GTK_BOX(cmd->new_parent), cmd->card, cmd->insert_after);
        
        int pos = 0;
        GtkWidget *c = gtk_widget_get_first_child(cmd->new_parent);
        while (c) {
            int cid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(c), "card_id"));
            if (cid > 0) db_update_card_position(cid, cmd->new_list_id, pos++);
            c = gtk_widget_get_next_sibling(c);
        }
    }
    
    if (cmd->card) g_object_unref(cmd->card);
    if (cmd->old_parent) g_object_unref(cmd->old_parent);
    if (cmd->new_parent) g_object_unref(cmd->new_parent);
    if (cmd->insert_after) g_object_unref(cmd->insert_after);
    g_free(cmd);
    
    return G_SOURCE_REMOVE;
}

static gboolean on_drop(GtkDropTarget *target, const GValue *value, double x, double y, gpointer data) {
    (void)target; (void)value; (void)x; (void)y;
    GtkWidget *card_list_box = GTK_WIDGET(data);
    GtkWidget *col_box = GTK_WIDGET(g_object_get_data(G_OBJECT(card_list_box), "col_box"));
    if (col_box) gtk_widget_remove_css_class(col_box, "drag-hover");
    
    if (!global_dragged_card) return FALSE;
    
    int new_list_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(card_list_box), "list_id"));
    
    GtkWidget *insert_after = NULL;
    if (placeholder_card && gtk_widget_get_parent(placeholder_card) == card_list_box) {
        insert_after = gtk_widget_get_prev_sibling(placeholder_card);
        gtk_box_remove(GTK_BOX(card_list_box), placeholder_card);
    } else {
        insert_after = gtk_widget_get_last_child(card_list_box);
    }
    
    if (insert_after == global_dragged_card) {
        insert_after = gtk_widget_get_prev_sibling(global_dragged_card);
    }
    
    GtkWidget *old_parent = gtk_widget_get_parent(global_dragged_card);
    
    CardMoveData *cmd = g_malloc(sizeof(CardMoveData));
    cmd->card = global_dragged_card;
    cmd->old_parent = old_parent;
    cmd->new_parent = card_list_box;
    cmd->insert_after = insert_after;
    cmd->new_list_id = new_list_id;
    
    if (cmd->card) g_object_ref(cmd->card);
    if (cmd->old_parent) g_object_ref(cmd->old_parent);
    if (cmd->new_parent) g_object_ref(cmd->new_parent);
    if (cmd->insert_after) g_object_ref(cmd->insert_after);
    
    g_idle_add(idle_move_card, cmd);
    
    return TRUE;
}

GtkWidget *global_dragged_list = NULL;

static void on_list_drag_begin(GtkDragSource *source, GdkDrag *drag, gpointer data) {
    (void)source; (void)drag;
    if (global_dragged_list) g_object_remove_weak_pointer(G_OBJECT(global_dragged_list), (gpointer *)&global_dragged_list);
    global_dragged_list = GTK_WIDGET(data);
    g_object_add_weak_pointer(G_OBJECT(global_dragged_list), (gpointer *)&global_dragged_list);
    gtk_widget_set_opacity(global_dragged_list, 0.4);
    
    GdkPaintable *paintable = gtk_widget_paintable_new(global_dragged_list);
    gtk_drag_source_set_icon(source, paintable, 0, 0);
    g_object_unref(paintable);
}

static void on_list_drag_end(GtkDragSource *source, GdkDrag *drag, gboolean delete_data, gpointer data) {
    (void)source; (void)drag; (void)delete_data; (void)data;
    if (global_dragged_list) {
        gtk_widget_set_opacity(global_dragged_list, 1.0);
        g_object_remove_weak_pointer(G_OBJECT(global_dragged_list), (gpointer *)&global_dragged_list);
    }
    global_dragged_list = NULL;
}

GtkWidget* ui_board_create_list(int list_id, const char *title) {
    GtkWidget *col_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(col_box, 10);
    gtk_widget_set_margin_end(col_box, 10);
    gtk_widget_set_margin_top(col_box, 10);
    gtk_widget_set_margin_bottom(col_box, 10);
    gtk_widget_set_size_request(col_box, 280, -1);
    gtk_widget_set_halign(col_box, GTK_ALIGN_START);
    gtk_widget_add_css_class(col_box, "list-container");
    gtk_widget_set_valign(col_box, GTK_ALIGN_START); 
    g_object_set_data(G_OBJECT(col_box), "list_id", GINT_TO_POINTER(list_id));
    
    gtk_widget_add_css_class(col_box, "view-dark");

    GtkDragSource *drag_source = gtk_drag_source_new();
    gtk_drag_source_set_actions(drag_source, GDK_ACTION_MOVE);
    char payload_str[32];
    snprintf(payload_str, sizeof(payload_str), "%d", list_id);
    GValue val = G_VALUE_INIT;
    g_value_init(&val, G_TYPE_STRING);
    g_value_set_string(&val, payload_str);
    GdkContentProvider *provider = gdk_content_provider_new_for_value(&val);
    g_value_unset(&val);
    gtk_drag_source_set_content(drag_source, provider);
    g_object_unref(provider);
    g_value_unset(&val);
    g_signal_connect(drag_source, "drag-begin", G_CALLBACK(on_list_drag_begin), col_box);
    g_signal_connect(drag_source, "drag-end", G_CALLBACK(on_list_drag_end), NULL);

    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(header_box, 8);
    gtk_widget_set_margin_end(header_box, 8);
    gtk_widget_set_margin_top(header_box, 4);
    gtk_box_append(GTK_BOX(col_box), header_box);
    
    // Attach drag source ONLY to the header to avoid intercepting card drags
    gtk_widget_add_controller(header_box, GTK_EVENT_CONTROLLER(drag_source));

    GtkWidget *title_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(title_entry), title);
    gtk_widget_set_hexpand(title_entry, TRUE);
    gtk_widget_add_css_class(title_entry, "list-title-entry");
    g_signal_connect(title_entry, "activate", G_CALLBACK(on_list_title_changed), col_box);
    GtkEventController *focus_ctrl = gtk_event_controller_focus_new();
    g_signal_connect(focus_ctrl, "leave", G_CALLBACK(on_list_title_focus_leave), col_box);
    gtk_widget_add_controller(title_entry, focus_ctrl);
    
    gtk_box_append(GTK_BOX(header_box), title_entry);

    GtkWidget *count_lbl = gtk_label_new("1");
    gtk_widget_add_css_class(count_lbl, "list-count-dark");
    gtk_box_append(GTK_BOX(header_box), count_lbl);

    GtkWidget *del_list_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
    gtk_widget_add_css_class(del_list_btn, "delete-btn-dark");
    g_signal_connect(del_list_btn, "clicked", G_CALLBACK(on_delete_list_clicked), col_box);
    gtk_box_append(GTK_BOX(header_box), del_list_btn);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scrolled), TRUE);
    gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(scrolled), 600);
    gtk_widget_add_css_class(scrolled, "list-scroll"); 
    gtk_widget_set_vexpand(scrolled, FALSE); 
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_append(GTK_BOX(col_box), scrolled);

    GtkWidget *card_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(card_list, 8);
    gtk_widget_set_margin_end(card_list, 8);
    g_object_set_data(G_OBJECT(card_list), "list_id", GINT_TO_POINTER(list_id));
    g_object_set_data(G_OBJECT(card_list), "col_box", col_box);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), card_list);

    GtkDropTarget *drop_target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_MOVE);
    g_signal_connect(drop_target, "drop", G_CALLBACK(on_drop), card_list);
    g_signal_connect(drop_target, "motion", G_CALLBACK(on_drag_motion), card_list);
    g_signal_connect(drop_target, "leave", G_CALLBACK(on_drag_leave), card_list);
    gtk_widget_add_controller(card_list, GTK_EVENT_CONTROLLER(drop_target));
    
    int card_count = 0;
    Card *cards = db_get_cards(list_id, &card_count);
    
    char count_str[16];
    snprintf(count_str, sizeof(count_str), "%d", card_count);
    gtk_label_set_text(GTK_LABEL(count_lbl), count_str);

    for (int i = 0; i < card_count; i++) {
        GtkWidget *c = ui_card_create(&cards[i]);
        gtk_box_append(GTK_BOX(card_list), c);
    }
    db_free_cards(cards);

    GtkWidget *footer_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(footer_box, 8);
    gtk_widget_set_margin_end(footer_box, 8);
    gtk_widget_set_margin_bottom(footer_box, 4);
    gtk_box_append(GTK_BOX(col_box), footer_box);

    GtkWidget *add_btn = gtk_button_new_with_label("+ Add a card");
    gtk_widget_set_hexpand(add_btn, TRUE);
    gtk_widget_set_halign(add_btn, GTK_ALIGN_START);
    gtk_widget_add_css_class(add_btn, "add-card-btn-dark");
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_card_clicked), card_list);
    gtk_box_append(GTK_BOX(footer_box), add_btn);
    
    // Template icon (mock)
    GtkWidget *template_btn = gtk_button_new_from_icon_name("document-new-symbolic");
    gtk_widget_add_css_class(template_btn, "add-card-btn-dark");
    gtk_box_append(GTK_BOX(footer_box), template_btn);

    return col_box;
}

static void on_add_list_save(GtkWidget *widget, gpointer user_data) {
    GtkWidget *stack = GTK_WIDGET(user_data);
    GtkWidget *entry = g_object_get_data(G_OBJECT(stack), "entry");
    GtkWidget *board_box = gtk_widget_get_parent(gtk_widget_get_parent(stack));
    int board_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(board_box), "board_id"));
    const char *title = gtk_editable_get_text(GTK_EDITABLE(entry));
    
    if (title && strlen(title) > 0) {
        int new_id = db_create_list(board_id, title, 0);
        if (new_id > 0) {
            GtkWidget *new_list = ui_board_create_list(new_id, title);
            GtkWidget *add_list_container = gtk_widget_get_last_child(board_box);
            
            g_object_ref(add_list_container);
            gtk_box_remove(GTK_BOX(board_box), add_list_container);
            gtk_box_append(GTK_BOX(board_box), new_list);
            gtk_box_append(GTK_BOX(board_box), add_list_container);
            g_object_unref(add_list_container);
        }
    }
    gtk_editable_set_text(GTK_EDITABLE(entry), "");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "button");
}

static void on_add_list_cancel(GtkButton *btn, gpointer user_data) {
    GtkWidget *stack = GTK_WIDGET(user_data);
    GtkWidget *entry = g_object_get_data(G_OBJECT(stack), "entry");
    gtk_editable_set_text(GTK_EDITABLE(entry), "");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "button");
}

static void on_add_list_clicked(GtkButton *btn, gpointer user_data) {
    GtkWidget *stack = GTK_WIDGET(user_data);
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "input");
    GtkWidget *entry = g_object_get_data(G_OBJECT(stack), "entry");
    gtk_widget_grab_focus(entry);
}

static GtkWidget *placeholder_list = NULL;
static void create_list_placeholder() {
    if (!placeholder_list) {
        placeholder_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_size_request(placeholder_list, 280, -1);
        gtk_widget_add_css_class(placeholder_list, "placeholder-list");
        g_object_ref_sink(placeholder_list);
    }
}

static GdkDragAction on_list_drag_motion(GtkDropTarget *target, double x, double y, gpointer data) {
    (void)target; (void)y;
    GtkWidget *board_box = GTK_WIDGET(data);
    
    if (!global_dragged_list) return GDK_ACTION_MOVE;
    create_list_placeholder();

    GtkWidget *parent = gtk_widget_get_parent(placeholder_list);
    if (parent != board_box) {
        if (parent) gtk_box_remove(GTK_BOX(parent), placeholder_list);
        gtk_box_append(GTK_BOX(board_box), placeholder_list);
    }
    
    GtkWidget *child = gtk_widget_get_first_child(board_box);
    GtkWidget *insert_before = NULL;
    
    while (child) {
        if (child != placeholder_list && child != global_dragged_list) {
            // Stop before the Add List button (it doesn't have a list_id)
            if (g_object_get_data(G_OBJECT(child), "list_id") == NULL) {
                if (!insert_before) insert_before = child;
                break;
            }
            graphene_point_t p;
            if (gtk_widget_compute_point(child, board_box, &GRAPHENE_POINT_INIT(0, 0), &p)) {
                int width = gtk_widget_get_width(child);
                if (x < p.x + width / 2.0) {
                    insert_before = child;
                    break;
                }
            }
        }
        child = gtk_widget_get_next_sibling(child);
    }
    
    GtkWidget *new_prev = insert_before ? gtk_widget_get_prev_sibling(insert_before) : gtk_widget_get_last_child(board_box);
    if (new_prev == global_dragged_list) new_prev = gtk_widget_get_prev_sibling(new_prev);
    if (new_prev == placeholder_list) new_prev = gtk_widget_get_prev_sibling(placeholder_list);
    
    GtkWidget *current_prev = gtk_widget_get_prev_sibling(placeholder_list);
    if (current_prev != new_prev) {
        gtk_box_reorder_child_after(GTK_BOX(board_box), placeholder_list, new_prev);
    }
    return GDK_ACTION_MOVE;
}

static gboolean idle_remove_placeholder_list(gpointer data) {
    (void)data;
    if (placeholder_list) {
        GtkWidget *parent = gtk_widget_get_parent(placeholder_list);
        if (parent) gtk_box_remove(GTK_BOX(parent), placeholder_list);
    }
    return G_SOURCE_REMOVE;
}

static void on_list_drag_leave(GtkDropTarget *target, gpointer data) {
    (void)target; (void)data;
    g_idle_add(idle_remove_placeholder_list, NULL);
}

typedef struct {
    GtkWidget *list;
    GtkWidget *old_parent;
    GtkWidget *new_parent;
    GtkWidget *insert_after;
} ListMoveData;

static gboolean idle_move_list(gpointer data) {
    ListMoveData *lmd = data;
    
    if (lmd->old_parent && GTK_IS_WIDGET(lmd->old_parent) && GTK_IS_WIDGET(lmd->list)) {
        gtk_box_remove(GTK_BOX(lmd->old_parent), lmd->list);
    }
    
    if (GTK_IS_WIDGET(lmd->new_parent) && GTK_IS_WIDGET(lmd->list)) {
        gtk_box_insert_child_after(GTK_BOX(lmd->new_parent), lmd->list, lmd->insert_after);
        
        int pos = 0;
        GtkWidget *c = gtk_widget_get_first_child(lmd->new_parent);
        while (c) {
            int lid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(c), "list_id"));
            if (lid > 0) db_update_list_position(lid, pos++);
            c = gtk_widget_get_next_sibling(c);
        }
    }
    
    if (lmd->list) g_object_unref(lmd->list);
    if (lmd->old_parent) g_object_unref(lmd->old_parent);
    if (lmd->new_parent) g_object_unref(lmd->new_parent);
    if (lmd->insert_after) g_object_unref(lmd->insert_after);
    g_free(lmd);
    
    return G_SOURCE_REMOVE;
}

static gboolean on_list_drop(GtkDropTarget *target, const GValue *value, double x, double y, gpointer data) {
    (void)target; (void)value; (void)x; (void)y;
    GtkWidget *board_box = GTK_WIDGET(data);
    
    if (!global_dragged_list) return FALSE;
    
    GtkWidget *insert_after = NULL;
    if (placeholder_list && gtk_widget_get_parent(placeholder_list) == board_box) {
        insert_after = gtk_widget_get_prev_sibling(placeholder_list);
        gtk_box_remove(GTK_BOX(board_box), placeholder_list);
    } else {
        insert_after = gtk_widget_get_last_child(board_box);
    }
    
    if (insert_after == global_dragged_list) {
        insert_after = gtk_widget_get_prev_sibling(global_dragged_list);
    }
    
    GtkWidget *old_parent = gtk_widget_get_parent(global_dragged_list);
    
    ListMoveData *lmd = g_malloc(sizeof(ListMoveData));
    lmd->list = global_dragged_list;
    lmd->old_parent = old_parent;
    lmd->new_parent = board_box;
    lmd->insert_after = insert_after;
    
    if (lmd->list) g_object_ref(lmd->list);
    if (lmd->old_parent) g_object_ref(lmd->old_parent);
    if (lmd->new_parent) g_object_ref(lmd->new_parent);
    if (lmd->insert_after) g_object_ref(lmd->insert_after);
    
    g_idle_add(idle_move_list, lmd);
    
    return TRUE;
}

GtkWidget* ui_board_create_view(int board_id) {
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);
    
    GtkWidget *board_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); // Spacing handled by margins now
    gtk_widget_set_margin_start(board_box, 15);
    gtk_widget_set_margin_top(board_box, 15);
    g_object_set_data(G_OBJECT(board_box), "board_id", GINT_TO_POINTER(board_id));
    
    // Drop Target for lists
    GtkDropTarget *drop_target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_MOVE);
    g_signal_connect(drop_target, "drop", G_CALLBACK(on_list_drop), board_box);
    g_signal_connect(drop_target, "motion", G_CALLBACK(on_list_drag_motion), board_box);
    g_signal_connect(drop_target, "leave", G_CALLBACK(on_list_drag_leave), board_box);
    gtk_widget_add_controller(board_box, GTK_EVENT_CONTROLLER(drop_target));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), board_box);

    int list_count = 0;
    List *lists = db_get_lists(board_id, &list_count);
    for (int i = 0; i < list_count; i++) {
        GtkWidget *l = ui_board_create_list(lists[i].id, lists[i].name);
        gtk_box_append(GTK_BOX(board_box), l);
    }
    db_free_lists(lists);

    // Add list button container
    GtkWidget *add_list_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(add_list_container, 280, -1);
    gtk_widget_set_valign(add_list_container, GTK_ALIGN_START);
    gtk_widget_set_margin_start(add_list_container, 10);
    gtk_widget_set_margin_end(add_list_container, 10);
    gtk_widget_set_margin_top(add_list_container, 10);
    
    GtkWidget *stack = gtk_stack_new();
    gtk_box_append(GTK_BOX(add_list_container), stack);
    
    // Page 1: Button
    GtkWidget *add_list_btn = gtk_button_new_with_label("+ Add another list");
    gtk_widget_add_css_class(add_list_btn, "add-list-btn");
    g_signal_connect(add_list_btn, "clicked", G_CALLBACK(on_add_list_clicked), stack);
    gtk_stack_add_named(GTK_STACK(stack), add_list_btn, "button");
    
    // Page 2: Input + Save/Cancel
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_add_css_class(input_box, "view-dark");
    gtk_widget_set_margin_start(input_box, 4);
    gtk_widget_set_margin_end(input_box, 4);
    gtk_widget_set_margin_top(input_box, 4);
    gtk_widget_set_margin_bottom(input_box, 4);
    
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter list title...");
    g_signal_connect(entry, "activate", G_CALLBACK(on_add_list_save), stack);
    gtk_box_append(GTK_BOX(input_box), entry);
    
    GtkWidget *actions_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *save_btn = gtk_button_new_with_label("Add list");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_add_list_save), stack);
    gtk_box_append(GTK_BOX(actions_box), save_btn);
    
    GtkWidget *cancel_btn = gtk_button_new_from_icon_name("window-close-symbolic");
    gtk_widget_add_css_class(cancel_btn, "flat");
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_add_list_cancel), stack);
    gtk_box_append(GTK_BOX(actions_box), cancel_btn);
    
    gtk_box_append(GTK_BOX(input_box), actions_box);
    gtk_stack_add_named(GTK_STACK(stack), input_box, "input");
    
    g_object_set_data(G_OBJECT(stack), "entry", entry);
    
    gtk_box_append(GTK_BOX(board_box), add_list_container);

    return scroll;
}
