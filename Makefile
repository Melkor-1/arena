CC := gcc-13

CFLAGS += -std=c2x
CFLAGS += -fPIC
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -Wwrite-strings
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
debug: CFLAGS += -g3 -ggdb 

debug release: $(TARGET)

clean: 
	$(RM) $(TARGET)

.PHONY: all release debug 
.DELETE_ON_ERROR:
