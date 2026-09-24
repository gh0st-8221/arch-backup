CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = arch-backup
SRC = arch-backup.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
