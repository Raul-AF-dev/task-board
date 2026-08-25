#ifndef UI_BOARD_H
#define UI_BOARD_H

#include <gtk/gtk.h>

GtkWidget* ui_board_create_view(int board_id);
GtkWidget* ui_board_create_list(int list_id, const char *title);

#endif // UI_BOARD_H
