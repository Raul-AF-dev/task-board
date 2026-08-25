#ifndef MODELS_H
#define MODELS_H

#include <stdbool.h>

#define MAX_NAME_LEN 128
#define MAX_DESC_LEN 2048
#define MAX_COLOR_LEN 16

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    char background_color[MAX_COLOR_LEN];
    char background_image[256];
} Board;

typedef struct {
    int id;
    int board_id;
    char name[MAX_NAME_LEN];
    int position;
} List;

typedef struct {
    int id;
    int list_id;
    char name[MAX_NAME_LEN];
    char description[MAX_DESC_LEN];
    char due_date[32]; // ISO format or just string
    int position;
} Card;

typedef struct {
    int id;
    int board_id;
    char name[MAX_NAME_LEN];
    char color[MAX_COLOR_LEN];
} Label;

typedef struct {
    int id;
    int card_id;
    char name[MAX_NAME_LEN];
    int position;
} Checklist;

typedef struct {
    int id;
    int checklist_id;
    char text[MAX_NAME_LEN];
    bool is_completed;
    int position;
} ChecklistItem;

#endif // MODELS_H
