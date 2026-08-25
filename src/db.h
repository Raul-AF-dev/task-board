#ifndef DB_H
#define DB_H

#include "models.h"
#include <sqlite3.h>
#include <stdbool.h>

extern sqlite3 *db;

bool db_init(const char *db_path);
void db_close(void);

// Boards
int db_create_board(const char *name, const char *bg_color);
Board* db_get_boards(int *count);
void db_delete_board(int id);
void db_update_board_name(int board_id, const char *new_name);
void db_update_board_background(int board_id, const char *image_path);

// Lists
int db_create_list(int board_id, const char *name, int position);
List* db_get_lists(int board_id, int *count);
void db_update_list_position(int list_id, int new_position);
void db_delete_list(int id);

// Cards
int db_create_card(int list_id, const char *name, int position);
Card* db_get_cards(int list_id, int *count);
void db_update_card(Card *card);
void db_update_card_position(int card_id, int new_list_id, int new_position);
void db_delete_card(int id);

// Free helpers
void db_free_boards(Board *boards);
void db_free_lists(List *lists);
void db_free_cards(Card *cards);

#endif // DB_H
void db_update_list_name(int list_id, const char *new_name);
