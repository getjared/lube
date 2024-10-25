CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm -lgif -ljpeg -lSDL2

TARGET = lube
SRCS = lube.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)