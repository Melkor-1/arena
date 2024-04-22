CC := gcc-13

CFLAGS += -std=c2x
CFLAGS += -fPIC
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -Wwrite-strings
CFLAGS += -Wno-unused-variable
CFLAGS += -Wno-parentheses
CFLAGS += -Wpedantic
CFLAGS += -Warray-bounds
CFLAGS += -Wno-unused-function
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wdeprecated

TARGET := arena
TEST_TARGET := tests

release: CFLAGS += -O2 -s -DTEST_MAIN
release: $(TARGET)

debug: CFLAGS += -DTEST_MAIN -DDEBUG -g3 -ggdb -fsanitize=address,leak,undefined
debug: $(TARGET)

test: CFLAGS += -DDEBUG
test: $(TEST_TARGET)
	./$(TEST_TARGET) --verbose=3 

# Add targets for a shared library and a static library.

clean: 
	$(RM) $(TARGET) $(TEST_TARGET)

.PHONY: release debug build-tests test clean
.DELETE_ON_ERROR:
