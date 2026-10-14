CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm

TARGET = bptree
SRCS = main.c bptree.c node.c file_io.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c bptree.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe
