CC      := gcc
CFLAGS  := -Wall -Wextra -std=c99 -g
TARGET  := cagerd

SRCS    := main.c oci_config.c cJSON.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
