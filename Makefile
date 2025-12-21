CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -pthread -g

SRC = src/scheduler.c src/scheduler_test.c src/arithmetic_test.c
OBJ = $(SRC:.c=.o)

all: scheduler_test arithmetic_test

# Builds scheduler_test binary
scheduler_test: src/scheduler.o src/scheduler_test.o
	$(CC) $(CFLAGS) -o $@ $^

# Builds arithmetic_test binary
arithmetic_test: src/scheduler.o src/arithmetic_test.o
	$(CC) $(CFLAGS) -o $@ $^

# Compiles all .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o scheduler_test arithmetic_test
