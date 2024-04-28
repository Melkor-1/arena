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

CFLAGS += $(EXTRA_CFLAGS)

TARGET = arena
TEST_TARGET = tests
SLIB_TARGET = libarena.a
DLIB_TARGET = libarena.so

RM = /bin/rm -f

static: $(SLIB_TARGET)

$(SLIB_TARGET): $(TARGET).o
	$(AR) rcs $@ $(TARGET).o

shared: $(DLIB_TARGET)

$(DLIB_TARGET): $(TARGET).o
	$(CC) $(CFLAGS) $(TARGET).o -o $@ $(LDFLAGS) -shared

test: 
	$(MAKE) EXTRA_CFLAGS="-DDEBUG" $(TEST_TARGET)
	./$(TEST_TARGET) --verbose=3

clean: 
	$(RM) $(TEST_TARGET) $(TARGET).o $(SLIB_TARGET) $(DLIB_TARGET)

.PHONY: release debug static shared test clean
.DELETE_ON_ERROR:
