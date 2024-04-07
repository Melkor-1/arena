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
CFLAGS += -Wconversion
CFLAGS += -Wdeprecated

CFLAGS += -DTEST_MAIN

TARGET := arena

release: CFLAGS += -O2 -s
release: $(TARGET)

debug: CFLAGS += -DDEBUG -g3 -ggdb -fanalyzer -fsanitize=address,leak,undefined
debug: $(TARGET)

clean: 
	$(RM) $(TARGET)

.PHONY: release debug clean
.DELETE_ON_ERROR:
