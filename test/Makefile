SOURCES=$(shell find ./ -type f -name '*_test.c')
TARGETS=$(shell find ../src/ -name '*.c' ! -name 'main.c')
TARGET_OBJECTS=$(subst src/,build/,$(TARGETS:.c=.o))

OBJECTS=$(subst ./,./build/,$(SOURCES:.c=.o))
TESTS=$(OBJECTS:.o=)

LDFLAGS=-lm $(shell pkg-config --libs igraph criterion cairo)
CFLAGS=-Wall -Wextra -ggdb -pedantic -std=gnu11 -I../src $(shell pkg-config --cflags igraph criterion cairo) -O2 -Wconversion -Wshadow -Wlogical-op -Wshift-overflow=2 -Wduplicated-cond -fstack-protector -Wstrict-prototypes

test-all: | $(TESTS)
	for test in $(TESTS); do $$test ; done

# $(TESTS): $(OBJECTS)

build:
	mkdir -p build

build/%: %.c $(TARGET_OBJECTS) | build
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: clean test-all

clean:
	rm -rfv build
