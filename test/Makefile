# Root Makefile in test directory

# Directories
SRC_DIR := ../src
BUILD_DIR := build

# Detect test directories automatically
TEST_DIRS := $(wildcard test*)

# Compiler and Flags
CC := gcc
CFLAGS := -Wall -Wextra -I$(SRC_DIR)
LDFLAGS := -lrdmacm -libverbs -pthread

# Source files
SRC_FILES := $(SRC_DIR)/client.c $(SRC_DIR)/server.c $(SRC_DIR)/helper.c $(SRC_DIR)/rupt.c

# Object files
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Targets for all tests
all: $(foreach dir,$(TEST_DIRS),$(BUILD_DIR)/$(dir)_client $(BUILD_DIR)/$(dir)_server)

# Build rules for each test
define TEST_RULES
$(BUILD_DIR)/$(1)_client: $(OBJ_FILES) $(1)/client.c
	$(CC) $(CFLAGS) $(1)/client.c $(OBJ_FILES) -o $$@ $(LDFLAGS)

$(BUILD_DIR)/$(1)_server: $(OBJ_FILES) $(1)/server.c
	$(CC) $(CFLAGS) $(1)/server.c $(OBJ_FILES) -o $$@ $(LDFLAGS)
endef

# Apply the rules to each test directory
$(foreach dir,$(TEST_DIRS),$(eval $(call TEST_RULES,$(dir))))

# Build the object files from the source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Target to run tests
run_tests: all
	./run_tests.sh

# Clean up build files
clean:
	rm -rf $(BUILD_DIR)/*.o $(foreach dir,$(TEST_DIRS),$(BUILD_DIR)/$(dir)_client $(BUILD_DIR)/$(dir)_server)

.PHONY: all clean run_tests
