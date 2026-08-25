#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <gtk/gtk.h>
#include "../models.h"

void ui_window_init(GtkApplication *app);
void ui_window_show_board(int board_id);

#endif // UI_WINDOW_H
