CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LDFLAGS = -pthread -lsqlite3

TARGET = rest_server
SRCS = server.c thread_pool.c http_handler.c db_handler.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build completato: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) rest_api.db

re: clean all

.PHONY: all clean re