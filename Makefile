CC := gcc-13

CFLAGS += -std=c2x
CFLAGS += -fPIC
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpedantic
CFLAGS += -Wwrite-strings
CFLAGS += -Wno-parentheses
CFLAGS += -Wpedantic
CFLAGS += -Warray-bounds
CFLAGS += -Wno-unused-function
CFLAGS += -Wstrict-prototypes

all: arena

test: CFLAGS += -DTEST_MAIN 
test: arena 

clean: 
	$(RM) arena

.PHONY: all test clean 
.DELETE_ON_ERROR:
