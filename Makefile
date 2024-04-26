CFLAGS += -std=c11
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
SLIB_TARGET := libarena.a
DLIB_TARGET := libarena.so

RM := /bin/rm -f

release: CFLAGS += -O2 -s -DTEST_MAIN
release: $(TARGET)

debug: CFLAGS += -DTEST_MAIN -DDEBUG -g3 -ggdb -fsanitize=address,leak,undefined
debug: $(TARGET)

static: $(SLIB_TARGET)

$(SLIB_TARGET): $(TARGET).o
	$(AR) rcs $@ $^

shared: $(DLIB_TARGET)
shared: LDFLAGS += -shared

$(DLIB_TARGET): $(TARGET).o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: CFLAGS += -DDEBUG
test: $(TEST_TARGET)
	./$(TEST_TARGET) --verbose=3

clean: 
	$(RM) $(TARGET) $(TEST_TARGET) $(TARGET).o $(SLIB_TARGET) $(DLIB_TARGET)

.PHONY: release debug static shared test clean
.DELETE_ON_ERROR:
