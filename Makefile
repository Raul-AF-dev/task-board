CC = gcc

CFLAGS = -Wall -Wextra -O2 -g $(shell pkg-config --cflags gtk4) -Isrc
LDFLAGS = $(shell pkg-config --libs gtk4) -lpthread -ldl

TARGET = task-board
SRCS = src/main.c \
       src/db.c \
       src/sqlite3.c \
       src/ui/window.c \
       src/ui/board.c \
       src/ui/card.c \
       src/ui/card_dialog.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
