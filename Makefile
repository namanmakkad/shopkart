# ShopKart Makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS =

# Detect Windows
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
    TARGET = shopkart.exe
    RM = del /Q
    RMDIR = rmdir /S /Q
    PATHSEP = \\
else
    TARGET = shopkart
    RM = rm -f
    RMDIR = rm -rf
    PATHSEP = /
endif

SRCDIR = src
OBJDIR = obj
TESTDIR = tests

SOURCES = $(SRCDIR)/main.c $(SRCDIR)/server.c $(SRCDIR)/http_utils.c $(SRCDIR)/render.c \
          $(SRCDIR)/product.c $(SRCDIR)/sort.c $(SRCDIR)/search.c $(SRCDIR)/hashmap.c \
          $(SRCDIR)/auth.c $(SRCDIR)/cart.c $(SRCDIR)/stack.c $(SRCDIR)/queue.c \
          $(SRCDIR)/linkedlist.c $(SRCDIR)/heap.c $(SRCDIR)/bst.c

OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean test sanitize

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
ifeq ($(OS),Windows_NT)
	if not exist $(OBJDIR) mkdir $(OBJDIR)
else
	mkdir -p $(OBJDIR)
endif

clean:
ifeq ($(OS),Windows_NT)
	-if exist $(OBJDIR) $(RMDIR) $(OBJDIR)
	-if exist $(TARGET) $(RM) $(TARGET)
	-if exist test_dsa.exe $(RM) test_dsa.exe
else
	$(RMDIR) $(OBJDIR)
	$(RM) $(TARGET)
	$(RM) test_dsa
endif

test: $(TESTDIR)/test_dsa.c $(SRCDIR)/sort.c $(SRCDIR)/search.c $(SRCDIR)/hashmap.c $(SRCDIR)/stack.c $(SRCDIR)/queue.c $(SRCDIR)/linkedlist.c $(SRCDIR)/heap.c $(SRCDIR)/bst.c
	$(CC) $(CFLAGS) -o test_dsa $^ $(LDFLAGS)
	./test_dsa

sanitize: $(SOURCES)
	$(CC) $(CFLAGS) -fsanitize=address,undefined -o $(TARGET) $^ $(LDFLAGS)
