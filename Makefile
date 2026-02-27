CC      := gcc
CFLAGS  := -Wall -Wextra -Werror -std=gnu99 -Isrc

TARGET  := app
SRC_DIR := src

SRCS := $(wildcard $(SRC_DIR)/*.c)

.PHONY: all clean fclean re

all: fuzzer

fuzzer: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $@
clean:
	@true

fclean:
	rm -f fuzzer

re: fclean all