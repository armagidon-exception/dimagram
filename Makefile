SOURCES=$(shell find src/ -type f -name '*.c')

OBJECTS=$(subst src/,build/,$(SOURCES:.c=.o))
HEADERS=$(shell find src/ -type f -name '*.h')

LDFLAGS=-lm -fsanitize=address,undefined $(shell pkg-config --libs cairo igraph)
CFLAGS=-Wall -Wextra -ggdb -pedantic -std=gnu11 -Isrc $(shell pkg-config --cflags cairo igraph) -O2 -Wconversion -Wshadow -Wlogical-op -Wshift-overflow=2 -Wduplicated-cond -fstack-protector -Wstrict-prototypes

dimagram: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

build:
	mkdir -p build

build/%.o: src/%.c build $(HEADERS) 
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rfv build dimagram
