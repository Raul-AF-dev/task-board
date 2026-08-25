#include "window.h"
#include "../db.h"
#include "board.h"
#include <stdio.h>
#include <string.h>

static GtkWidget *main_window = NULL;
static GtkWidget *main_stack = NULL;
static GtkWidget *home_page = NULL;
static GtkWidget *board_page = NULL;
static GtkWidget *boards_flow = NULL;
static GtkWidget *board_content_area = NULL;
static GtkWidget *board_title_entry = NULL;
static int current_board_id = -1;

// Forward declaration
void ui_window_show_home(void);

static void on_board_card_clicked(GtkButton *btn, gpointer user_data) {
    int board_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "board_id"));
    ui_window_show_board(board_id);
}



static void on_new_board_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    int new_id = db_create_board("New Project", "#282e33");
    if (new_id > 0) {
        ui_window_show_board(new_id);
    }
}

static GtkCssProvider *board_bg_provider = NULL;

static void update_board_background(const char *image_path) {
    if (!board_bg_provider) {
        board_bg_provider = gtk_css_provider_new();
        gtk_style_context_add_provider_for_display(
            gdk_display_get_default(),
            GTK_STYLE_PROVIDER(board_bg_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1
        );
    }
    
    char css[1024];
    if (image_path && strlen(image_path) > 0) {
        if (image_path[0] == '#') {
            snprintf(css, sizeof(css), ".board-page { background-image: none; background-color: %s; }", image_path);
        } else {
            snprintf(css, sizeof(css), ".board-page { background-image: url('file://%s'); background-size: cover; background-position: center; }", image_path);
        }
    } else {
        snprintf(css, sizeof(css), ".board-page { background-image: none; background-color: #1d2125; }");
    }
    gtk_css_provider_load_from_string(board_bg_provider, css);
}

static void on_bg_response(GtkDialog *dialog, int response, gpointer user_data) {
    (void)user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
        if (file) {
            char *path = g_file_get_path(file);
            if (path && current_board_id != -1) {
                db_update_board_background(current_board_id, path);
                update_board_background(path);
                g_free(path);
            }
            g_object_unref(file);
        }
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
}


static void on_color_picked(GtkButton *btn, gpointer user_data) {
    const char *color_hex = (const char *)user_data;
    if (current_board_id != -1) {
        db_update_board_background(current_board_id, color_hex);
        ui_window_show_board(current_board_id);
    }
}
static void on_change_bg_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Choose Background Image",
                                                    GTK_WINDOW(main_window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    g_signal_connect(dialog, "response", G_CALLBACK(on_bg_response), NULL);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_board_title_changed(GtkEditable *editable, gpointer user_data) {
    (void)user_data;
    if (current_board_id == -1) return;
    const char *new_title = gtk_editable_get_text(editable);
    db_update_board_name(current_board_id, new_title);
}

static void on_home_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    ui_window_show_home();
}

void ui_window_show_home(void) {
    // Clear flow box
    GtkWidget *child = gtk_widget_get_first_child(boards_flow);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_flow_box_remove(GTK_FLOW_BOX(boards_flow), child);
        child = next;
    }

    // Load boards
    int count = 0;
    Board *boards = db_get_boards(&count);
    
    for (int i = 0; i < count; i++) {
        GtkWidget *btn = gtk_button_new();
        gtk_widget_set_halign(btn, GTK_ALIGN_FILL);
        gtk_widget_set_valign(btn, GTK_ALIGN_FILL);
        gtk_widget_add_css_class(btn, "board-card");
        
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_halign(box, GTK_ALIGN_FILL);
        gtk_widget_set_valign(box, GTK_ALIGN_FILL);
        gtk_widget_set_hexpand(box, TRUE);
        gtk_widget_set_vexpand(box, TRUE);
        
        GtkWidget *image_area;
        if (boards[i].background_image && strlen(boards[i].background_image) > 0) {
            if (boards[i].background_image[0] == '#') {
                image_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
                GtkCssProvider *provider = gtk_css_provider_new();
                char css[256];
                snprintf(css, sizeof(css), "box { background-color: %s; }", boards[i].background_image);
                gtk_css_provider_load_from_data(provider, css, -1);
                gtk_style_context_add_provider(gtk_widget_get_style_context(image_area), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
                g_object_unref(provider);
            } else {
                image_area = gtk_picture_new_for_filename(boards[i].background_image);
                gtk_picture_set_can_shrink(GTK_PICTURE(image_area), TRUE);
                gtk_picture_set_content_fit(GTK_PICTURE(image_area), GTK_CONTENT_FIT_COVER);
            }
        } else {
            image_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            GtkCssProvider *provider = gtk_css_provider_new();
            gtk_css_provider_load_from_data(provider, "box { background-color: #1d2125; }", -1);
            gtk_style_context_add_provider(gtk_widget_get_style_context(image_area), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
            g_object_unref(provider);
        }
        gtk_widget_add_css_class(image_area, "board-card-image");
        gtk_widget_set_vexpand(image_area, TRUE);
        gtk_widget_set_hexpand(image_area, TRUE);
        gtk_box_append(GTK_BOX(box), image_area);
        
        GtkWidget *label_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_add_css_class(label_box, "board-card-label-box");
        GtkWidget *label = gtk_label_new(boards[i].name);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        char *markup = g_markup_printf_escaped("<span font_desc='12' weight='bold' color='#b6c2cf'>%s</span>", boards[i].name);
        gtk_label_set_markup(GTK_LABEL(label), markup);
        g_free(markup);
        gtk_widget_set_margin_start(label, 12);
        gtk_widget_set_margin_end(label, 12);
        gtk_widget_set_margin_top(label, 8);
        gtk_widget_set_margin_bottom(label, 8);
        gtk_box_append(GTK_BOX(label_box), label);
        
        gtk_box_append(GTK_BOX(box), label_box);
        
        gtk_button_set_child(GTK_BUTTON(btn), box);
        g_object_set_data(G_OBJECT(btn), "board_id", GINT_TO_POINTER(boards[i].id));
        g_signal_connect(btn, "clicked", G_CALLBACK(on_board_card_clicked), NULL);
        
        GtkWidget *aspect = gtk_aspect_frame_new(0.5, 0.5, 2.5, FALSE);
        gtk_aspect_frame_set_child(GTK_ASPECT_FRAME(aspect), btn);
        gtk_flow_box_insert(GTK_FLOW_BOX(boards_flow), aspect, -1);
    }
    
    db_free_boards(boards);
    
    // Add "Create new board" card
    GtkWidget *create_btn = gtk_button_new();
    gtk_widget_set_halign(create_btn, GTK_ALIGN_FILL);
    gtk_widget_set_valign(create_btn, GTK_ALIGN_FILL);
    gtk_widget_add_css_class(create_btn, "board-card-create");
    GtkWidget *create_label = gtk_label_new("+ Create new board");
    gtk_button_set_child(GTK_BUTTON(create_btn), create_label);
    g_signal_connect(create_btn, "clicked", G_CALLBACK(on_new_board_clicked), NULL);
    
    GtkWidget *create_aspect = gtk_aspect_frame_new(0.5, 0.5, 2.5, FALSE);
    gtk_aspect_frame_set_child(GTK_ASPECT_FRAME(create_aspect), create_btn);
    gtk_flow_box_insert(GTK_FLOW_BOX(boards_flow), create_aspect, -1);

    gtk_stack_set_visible_child(GTK_STACK(main_stack), home_page);
    current_board_id = -1;
}

void ui_window_show_board(int board_id) {
    current_board_id = board_id;
    
    // Fetch board to get name and bg
    int count = 0;
    Board *boards = db_get_boards(&count);
    char current_name[MAX_NAME_LEN] = "Board";
    char current_bg[256] = "";
    for (int i = 0; i < count; i++) {
        if (boards[i].id == board_id) {
            strncpy(current_name, boards[i].name, MAX_NAME_LEN - 1);
            strncpy(current_bg, boards[i].background_image, 255);
            break;
        }
    }
    db_free_boards(boards);

    update_board_background(current_bg);

    // Set title entry without triggering update
    g_signal_handlers_block_by_func(board_title_entry, on_board_title_changed, NULL);
    gtk_editable_set_text(GTK_EDITABLE(board_title_entry), current_name);
    g_signal_handlers_unblock_by_func(board_title_entry, on_board_title_changed, NULL);

    // Rebind delete button to correct board_id
    // This is handled by passing board_id via data to delete button but it's cleaner
    // to just re-create the delete button or store it globally.
    // Instead of doing that, I'll store the board_id on the delete button widget which is in the header.
    // Actually, I'll do it later in window_init.

    // Clear board content area
    GtkWidget *child = gtk_widget_get_first_child(board_content_area);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(board_content_area), child);
        child = next;
    }

    // Load board view
    GtkWidget *board_view = ui_board_create_view(board_id);
    gtk_widget_set_vexpand(board_view, TRUE);
    gtk_widget_set_hexpand(board_view, TRUE);
    gtk_box_append(GTK_BOX(board_content_area), board_view);

    gtk_stack_set_visible_child(GTK_STACK(main_stack), board_page);
}

static void on_delete_current_board_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    if (current_board_id != -1) {
        db_delete_board(current_board_id);
        ui_window_show_home();
    }
}

void ui_window_init(GtkApplication *app) {
    main_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(main_window), "Task Board");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1200, 800);

    main_stack = gtk_stack_new();
    gtk_window_set_child(GTK_WINDOW(main_window), main_stack);

    // --- HOME PAGE ---
    home_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(home_page, 40);
    gtk_widget_set_margin_top(home_page, 40);
    
    GtkWidget *home_title = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(home_title), "<span font_desc='24' weight='bold' color='white'>Your Workspaces</span>");
    gtk_widget_set_halign(home_title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(home_page), home_title);

    GtkWidget *boards_scroll = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(boards_scroll, TRUE);
    gtk_widget_set_vexpand(boards_scroll, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(boards_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    boards_flow = gtk_flow_box_new();
    gtk_widget_set_valign(boards_flow, GTK_ALIGN_START);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(boards_flow), GTK_SELECTION_NONE);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(boards_flow), 2);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(boards_flow), 4);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(boards_flow), TRUE);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(boards_flow), 15);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(boards_flow), 15);
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(boards_scroll), boards_flow);
    gtk_box_append(GTK_BOX(home_page), boards_scroll);
    
    gtk_stack_add_named(GTK_STACK(main_stack), home_page, "home");

    // --- BOARD PAGE ---
    board_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(board_page, "board-page");
    
    // Top Bar
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_add_css_class(top_bar, "top-bar");
    gtk_widget_set_margin_start(top_bar, 10);
    gtk_widget_set_margin_end(top_bar, 10);
    gtk_widget_set_margin_top(top_bar, 10);
    gtk_widget_set_margin_bottom(top_bar, 10);
    gtk_box_append(GTK_BOX(board_page), top_bar);
    
    GtkWidget *home_btn = gtk_button_new_from_icon_name("go-home-symbolic");
    g_signal_connect(home_btn, "clicked", G_CALLBACK(on_home_clicked), NULL);
    gtk_box_append(GTK_BOX(top_bar), home_btn);
    
    board_title_entry = gtk_entry_new();
    gtk_widget_add_css_class(board_title_entry, "board-title-entry");
    gtk_widget_set_hexpand(board_title_entry, TRUE);
    g_signal_connect(board_title_entry, "changed", G_CALLBACK(on_board_title_changed), NULL);
    gtk_box_append(GTK_BOX(top_bar), board_title_entry);
    
    GtkWidget *change_bg_btn = gtk_menu_button_new();
    gtk_menu_button_set_label(GTK_MENU_BUTTON(change_bg_btn), "Change Background");
    
    GtkWidget *bg_popover = gtk_popover_new();
    GtkWidget *bg_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_popover_set_child(GTK_POPOVER(bg_popover), bg_box);
    
    GtkWidget *color_grid = gtk_flow_box_new();
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(color_grid), 4);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(color_grid), GTK_SELECTION_NONE);
    
    static const char *colors[] = {"#d29034", "#4bbf6b", "#0079bf", "#b04632", "#89609e", "#51e898", "#00c2e0", "#cd5a91"};
    for (int i=0; i<8; i++) {
        GtkWidget *cbtn = gtk_button_new();
        gtk_widget_set_size_request(cbtn, 40, 40);
        char css[128];
        snprintf(css, sizeof(css), "button { background-image: none; background: %s; border-radius: 4px; border: none; }", colors[i]);
        GtkCssProvider *p = gtk_css_provider_new();
        gtk_css_provider_load_from_string(p, css);
        gtk_style_context_add_provider(gtk_widget_get_style_context(cbtn), GTK_STYLE_PROVIDER(p), GTK_STYLE_PROVIDER_PRIORITY_USER);
        g_signal_connect(cbtn, "clicked", G_CALLBACK(on_color_picked), (gpointer)colors[i]);
        gtk_flow_box_insert(GTK_FLOW_BOX(color_grid), cbtn, -1);
    }
    gtk_box_append(GTK_BOX(bg_box), color_grid);
    
    GtkWidget *custom_img_btn = gtk_button_new_with_label("Choose Image...");
    g_signal_connect(custom_img_btn, "clicked", G_CALLBACK(on_change_bg_clicked), NULL);
    gtk_box_append(GTK_BOX(bg_box), custom_img_btn);
    
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(change_bg_btn), bg_popover);
    gtk_box_append(GTK_BOX(top_bar), change_bg_btn);
    
    GtkWidget *del_board_btn = gtk_button_new_with_label("Delete Board");
    gtk_widget_add_css_class(del_board_btn, "destructive-action");
    g_signal_connect(del_board_btn, "clicked", G_CALLBACK(on_delete_current_board_clicked), NULL);
    gtk_box_append(GTK_BOX(top_bar), del_board_btn);

    // Main Content
    board_content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(board_content_area, TRUE);
    gtk_widget_set_hexpand(board_content_area, TRUE);
    gtk_box_append(GTK_BOX(board_page), board_content_area);

    gtk_stack_add_named(GTK_STACK(main_stack), board_page, "board");

    // Show home initially
    ui_window_show_home();

    gtk_window_present(GTK_WINDOW(main_window));
}
