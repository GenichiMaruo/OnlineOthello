CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

SRCDIR = ./src
OBJDIR = obj

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

all: client_app.out

client_app.out: $(OBJS)
	@echo "Linking object files to create client_app.out..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: client_app.out"

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled $< to $@"

clean:
	@echo "Cleaning up build files..."
	rm -rf $(OBJDIR) client_app.out
	@echo "Clean complete."