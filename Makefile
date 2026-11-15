CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -O2
LDLIBS = -lm

TARGET = tp2
SRCS = main.c grafo.c
OBJS = $(SRCS:.c=.o)
HEADERS = grafo.h

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	python3 validar.py
