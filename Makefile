TARGET := main
CC := gcc
CFLAGS := -Wall -Werror -std=c99 -g
INCLUDES := -Iincludes
SRC_DIR := src
OBJ_DIR := build

# ------------ Sources / Objects -----------
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# ------------ Defualt Target --------------
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# ---------- Object File Rules -------------
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ---------- Create Build Dir --------------
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ---------- Convenience Targets -----------
.PHONY: clean run debug

run: $(TARGET)
	./$(TARGET)

debug: $(TARGET)
	gdb ./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
