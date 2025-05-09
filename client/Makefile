CC = gcc
CFLAGS = -Wall -g -pthread
LDFLAGS =

SRCDIR = ./src
OBJDIR = obj

OUTFILE = client_app.out
COPY_TARGETS = ./src/othello-front/middleware/bin ./src/othello-front/dist/middleware/bin

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

.PHONY: all clean copy_targets

all: $(OUTFILE) copy_targets

$(OUTFILE): $(OBJS)
	@echo "Linking object files to create $(OUTFILE)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(OUTFILE)"

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled $< to $@"

copy_targets: $(OUTFILE)
	@echo "Copying $(OUTFILE) to target directories..."
	@for dir in $(COPY_TARGETS); do \
		mkdir -p $$dir; \
		cp -v $(OUTFILE) $$dir/; \
	done
	@echo "Copy complete."

clean:
	@echo "Cleaning up build files..."
	rm -rf $(OBJDIR) $(OUTFILE)
	@echo "Clean complete."
