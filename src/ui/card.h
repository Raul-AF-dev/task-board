#ifndef UI_CARD_H
#define UI_CARD_H

#include <gtk/gtk.h>
#include "../models.h"

GtkWidget* ui_card_create(Card *card);
extern GtkWidget *global_dragged_card;

#endif // UI_CARD_H
