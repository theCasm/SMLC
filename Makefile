# Any copyright is dedicated to the Public Domain.
# https://creativecommons.org/publicdomain/zero/1.0/

TARGET := sml

BUILD_DIR := ./build
SRC_DIR := src

CC := clang

CFLAGS := -Wall -Wextra -g
LDFLAGS := -lm

SRCS := $(shell find $(SRC_DIR) -name '*.c' -or -name '*.asm')
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
