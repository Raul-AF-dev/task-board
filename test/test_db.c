#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/db.h"

int tests_passed = 0;
int tests_failed = 0;

#define EXPECT_TRUE(cond) do { \
    if (cond) { tests_passed++; } \
    else { tests_failed++; printf("FAIL: %s:%d: Expected %s to be true\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define EXPECT_EQ(a, b) do { \
    if ((a) == (b)) { tests_passed++; } \
    else { tests_failed++; printf("FAIL: %s:%d: Expected %d == %d\n", __FILE__, __LINE__, (int)(a), (int)(b)); } \
} while (0)

#define EXPECT_STR_EQ(a, b) do { \
    if (strcmp((a), (b)) == 0) { tests_passed++; } \
    else { tests_failed++; printf("FAIL: %s:%d: Expected '%s' == '%s'\n", __FILE__, __LINE__, (a), (b)); } \
} while (0)


void test_board_crud() {
    printf("Running test_board_crud...\n");
    int count = 0;
    
    // Create
    int board_id = db_create_board("Test Board", "#ffffff");
    EXPECT_TRUE(board_id > 0);
    
    // Read
    Board *boards = db_get_boards(&count);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(boards[0].id, board_id);
    EXPECT_STR_EQ(boards[0].name, "Test Board");
    EXPECT_STR_EQ(boards[0].background_color, "#ffffff");
    
    // Update Name
    db_update_board_name(board_id, "Updated Board");
    db_free_boards(boards);
    boards = db_get_boards(&count);
    EXPECT_STR_EQ(boards[0].name, "Updated Board");
    
    // Update Background
    db_update_board_background(board_id, "/path/to/img.png");
    db_free_boards(boards);
    boards = db_get_boards(&count);
    EXPECT_STR_EQ(boards[0].background_image, "/path/to/img.png");
    
    // Delete
    db_delete_board(board_id);
    db_free_boards(boards);
    boards = db_get_boards(&count);
    EXPECT_EQ(count, 0);
    
    printf("Finished test_board_crud.\n");
}

void test_list_crud() {
    printf("Running test_list_crud...\n");
    int board_id = db_create_board("List Test Board", "");
    
    int list1 = db_create_list(board_id, "To Do", 0);
    int list2 = db_create_list(board_id, "Doing", 1);
    
    int count = 0;
    List *lists = db_get_lists(board_id, &count);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(lists[0].id, list1);
    EXPECT_EQ(lists[1].id, list2);
    
    // Test ordering
    db_update_list_position(list1, 1);
    db_update_list_position(list2, 0);
    
    db_free_lists(lists);
    lists = db_get_lists(board_id, &count);
    // SQLite ORDER BY position ASC means list2 should be first
    EXPECT_EQ(lists[0].id, list2);
    EXPECT_EQ(lists[1].id, list1);
    
    db_delete_list(list1);
    db_free_lists(lists);
    lists = db_get_lists(board_id, &count);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(lists[0].id, list2);
    
    db_delete_list(list2);
    db_free_lists(lists);
    db_delete_board(board_id);
    printf("Finished test_list_crud.\n");
}

void test_card_crud() {
    printf("Running test_card_crud...\n");
    int board_id = db_create_board("Card Test Board", "");
    int list_id = db_create_list(board_id, "List", 0);
    
    int card1 = db_create_card(list_id, "Card 1", 0);
    int card2 = db_create_card(list_id, "Card 2", 1);
    
    int count = 0;
    Card *cards = db_get_cards(list_id, &count);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(cards[0].id, card1);
    
    // Update card (description, due date)
    cards[0].description[0] = '\0';
    strcpy(cards[0].description, "Test description");
    strcpy(cards[0].due_date, "2026-10-31");
    db_update_card(&cards[0]);
    
    db_free_cards(cards);
    cards = db_get_cards(list_id, &count);
    EXPECT_STR_EQ(cards[0].description, "Test description");
    EXPECT_STR_EQ(cards[0].due_date, "2026-10-31");
    
    // Move to new list
    int list2 = db_create_list(board_id, "List 2", 1);
    db_update_card_position(card1, list2, 0);
    
    db_free_cards(cards);
    cards = db_get_cards(list_id, &count);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(cards[0].id, card2);
    db_free_cards(cards);
    
    cards = db_get_cards(list2, &count);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(cards[0].id, card1);
    db_free_cards(cards);
    
    db_delete_board(board_id); // Cleanup would normally cascade, but let's test db_delete_card explicitly just in case
    db_delete_card(card1);
    db_delete_card(card2);
    printf("Finished test_card_crud.\n");
}

int main() {
    // Use an in-memory database or a separate test file so we don't mess up production data
    remove("test.db");
    if (!db_init("test.db")) {
        printf("Failed to init test DB!\n");
        return 1;
    }
    
    test_board_crud();
    test_list_crud();
    test_card_crud();
    
    db_close();
    remove("test.db");
    
    printf("\nTest Results: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
