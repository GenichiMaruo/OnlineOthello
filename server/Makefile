CC = gcc
CFLAGS = -Wall -g -pthread
LDFLAGS =

SRCDIR = ./src
OBJDIR = obj

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

all: server_app.out

server_app.out: $(OBJS)
	@echo "Linking object files to create server_app.out..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: server_app.out"

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled $< to $@"

clean:
	@echo "Cleaning up build files..."
	rm -rf $(OBJDIR) server_app.out
	@echo "Clean complete."