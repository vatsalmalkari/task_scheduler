CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS = -pthread

SRCDIR = src
INCDIR = include
BUILDDIR = build

# Source files (main + src/*.c)
SRCS = main.c $(wildcard $(SRCDIR)/*.c)

# Object files in build/ (e.g., build/main.o, build/scheduler.o)
OBJS = $(patsubst %.c,$(BUILDDIR)/%.o,$(notdir $(SRCS)))

TARGET = scheduler_app

.PHONY: all clean

all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

# Final binary
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile .c files into build/*.o
$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET)
# Remove all object files and the target binary