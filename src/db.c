#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

sqlite3 *db = NULL;

bool db_init(const char *db_path) {
    if (sqlite3_open(db_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return false;
    }

    const char *schema = 
        "CREATE TABLE IF NOT EXISTS boards ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  background_color TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS lists ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  board_id INTEGER NOT NULL,"
        "  name TEXT NOT NULL,"
        "  position INTEGER,"
        "  FOREIGN KEY(board_id) REFERENCES boards(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS cards ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  list_id INTEGER NOT NULL,"
        "  name TEXT NOT NULL,"
        "  description TEXT,"
        "  due_date TEXT,"
        "  position INTEGER,"
        "  FOREIGN KEY(list_id) REFERENCES lists(id) ON DELETE CASCADE"
        ");";

    char *err_msg = NULL;
    if (sqlite3_exec(db, schema, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }

    // Try to add column if it doesn't exist (ignore error)
    sqlite3_exec(db, "ALTER TABLE boards ADD COLUMN background_image TEXT;", 0, 0, 0);

    // Enable foreign keys
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, 0);

    return true;
}

void db_close(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

int db_create_board(const char *name, const char *bg_color) {
    const char *sql = "INSERT INTO boards (name, background_color, background_image) VALUES (?, ?, '');";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, bg_color ? bg_color : "#ffffff", -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            int id = sqlite3_last_insert_rowid(db);
            sqlite3_finalize(stmt);
            return id;
        }
    }
    sqlite3_finalize(stmt);
    return -1;
}

Board* db_get_boards(int *count) {
    const char *sql = "SELECT id, name, background_color, background_image FROM boards ORDER BY id;";
    sqlite3_stmt *stmt;
    Board *boards = NULL;
    *count = 0;
    int capacity = 10;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        boards = malloc(sizeof(Board) * capacity);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (*count >= capacity) {
                capacity *= 2;
                boards = realloc(boards, sizeof(Board) * capacity);
            }
            boards[*count].id = sqlite3_column_int(stmt, 0);
            strncpy(boards[*count].name, (const char*)sqlite3_column_text(stmt, 1), MAX_NAME_LEN-1);
            strncpy(boards[*count].background_color, (const char*)sqlite3_column_text(stmt, 2), MAX_COLOR_LEN-1);
            
            const char *bg_img = (const char*)sqlite3_column_text(stmt, 3);
            strncpy(boards[*count].background_image, bg_img ? bg_img : "", 255);
            
            (*count)++;
        }
    }
    sqlite3_finalize(stmt);
    return boards;
}

void db_delete_board(int id) {
    const char *sql = "DELETE FROM boards WHERE id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void db_update_board_name(int board_id, const char *new_name) {
    const char *sql = "UPDATE boards SET name = ? WHERE id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, board_id);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void db_update_board_background(int board_id, const char *image_path) {
    const char *sql = "UPDATE boards SET background_image = ? WHERE id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, image_path, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, board_id);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

int db_create_list(int board_id, const char *name, int position) {
    const char *sql = "INSERT INTO lists (board_id, name, position) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, board_id);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, position);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            int id = sqlite3_last_insert_rowid(db);
            sqlite3_finalize(stmt);
            return id;
        }
    }
    sqlite3_finalize(stmt);
    return -1;
}

List* db_get_lists(int board_id, int *count) {
    const char *sql = "SELECT id, board_id, name, position FROM lists WHERE board_id = ? ORDER BY position;";
    sqlite3_stmt *stmt;
    List *lists = NULL;
    *count = 0;
    int capacity = 10;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, board_id);
        lists = malloc(sizeof(List) * capacity);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (*count >= capacity) {
                capacity *= 2;
                lists = realloc(lists, sizeof(List) * capacity);
            }
            lists[*count].id = sqlite3_column_int(stmt, 0);
            lists[*count].board_id = sqlite3_column_int(stmt, 1);
            strncpy(lists[*count].name, (const char*)sqlite3_column_text(stmt, 2), MAX_NAME_LEN-1);
            lists[*count].position = sqlite3_column_int(stmt, 3);
            (*count)++;
        }
    }
    sqlite3_finalize(stmt);
    return lists;
}

void db_update_list_position(int list_id, int new_position) {
    const char *sql = "UPDATE lists SET position = ? WHERE id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, new_position);
        sqlite3_bind_int(stmt, 2, list_id);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void db_delete_list(int id) {
    const char *sql = "DELETE FROM lists WHERE id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

int db_create_card(int list_id, const char *name, int position) {
    const char *sql = "INSERT INTO cards (list_id, name, position, description, due_date) VALUES (?, ?, ?, '', '');";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, list_id);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, position);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            int id = sqlite3_last_insert_rowid(db);
            sqlite3_finalize(stmt);
            return id;
        }
    }
    sqlite3_finalize(stmt);
    return -1;
}

Card* db_get_cards(int list_id, int *count) {
    const char *sql = "SELECT id, list_id, name, description, due_date, position FROM cards WHERE list_id = ? ORDER BY position;";
    sqlite3_stmt *stmt;
    Card *cards = NULL;
    *count = 0;
    int capacity = 10;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, list_id);
        cards = malloc(sizeof(Card) * capacity);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (*count >= capacity) {
                capacity *= 2;
                cards = realloc(cards, sizeof(Card) * capacity);
            }
            cards[*count].id = sqlite3_column_int(stmt, 0);
            cards[*count].list_id = sqlite3_column_int(stmt, 1);
            strncpy(cards[*count].name, (const char*)sqlite3_column_text(stmt, 2), MAX_NAME_LEN-1);
            
            const char *desc = (const char*)sqlite3_column_text(stmt, 3);
            strncpy(cards[*count].description, desc ? desc : "", MAX_DESC_LEN-1);
            
            const char *due = (const char*)sqlite3_column_text(stmt, 4);
            strncpy(cards[*count].due_date, due ? due : "", 31);
            
            cards[*count].position = sqlite3_column_int(stmt, 5);
            (*count)++;
        }
    }
    sqlite3_finalize(stmt);
    return cards;
}

void db_update_card(Card *card) {
    const char *sql = "UPDATE cards SET name = ?, description = ?, due_date = ? WHERE id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, card->name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, card->description, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, card->due_date, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, card->id);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void db_update_card_position(int card_id, int new_list_id, int new_position) {
    const char *sql = "UPDATE cards SET list_id = ?, position = ? WHERE id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, new_list_id);
        sqlite3_bind_int(stmt, 2, new_position);
        sqlite3_bind_int(stmt, 3, card_id);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void db_delete_card(int id) {
    const char *sql = "DELETE FROM cards WHERE id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void db_free_boards(Board *boards) {
    if (boards) free(boards);
}

void db_free_lists(List *lists) {
    if (lists) free(lists);
}

void db_free_cards(Card *cards) {
    if (cards) free(cards);
}

void db_update_list_name(int list_id, const char *new_name) {
    const char *sql = "UPDATE lists SET name = ? WHERE id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, list_id);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}
