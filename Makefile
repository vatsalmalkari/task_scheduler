CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -pthread -g

# Mapping your exact files to Object files
# Ensure these match your actual filenames in the src folder!
SCHEDULER_CORE = src/scheduler.o
APP_TASKS = src/app_tasks.o
ARITHMETIC_TEST = src/arithmetic_test.o
MAIN_ENTRY = main.o

all: scheduler_app

# The final binary links everything together
scheduler_app: $(MAIN_ENTRY) $(SCHEDULER_CORE) $(APP_TASKS) $(ARITHMETIC_TEST)
	$(CC) $(CFLAGS) -o $@ $^

# Rule for src directory
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for root directory (main.c)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o *.o scheduler_app