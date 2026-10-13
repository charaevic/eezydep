CC = gcc
CFLAGS = -Wall -Wextra -I include
LDFLAGS =
SRC = $(filter-out src/test_%, $(wildcard src/*.c))
OBJ = $(SRC:.c=.o)
TARGET = proxy

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: tests/test_http_parser
	./tests/test_http_parser

tests/test_http_parser: tests/test_http_parser.c src/http_parse.c
	$(CC) $(CFLAGS) -o $@ $^

integration:
	bash tests/integration.sh

debug: CFLAGS += -g -fsanitize=address
debug: clean all

clean:
	rm -f $(OBJ) $(TARGET) tests/test_parser

.PHONY: all test integration debug clean