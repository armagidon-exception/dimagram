SOURCES=$(shell find src/ -type f -name '*.c')

OBJECTS=$(subst src/,build/,$(SOURCES:.c=.o))
HEADERS=$(shell find src/ -type f -name '*.h')
LDFLAGS=-lm  $(shell pkg-config --libs cairo igraph)
CFLAGS=-Wall -Wextra -ggdb -pedantic -std=gnu11 -Isrc $(shell pkg-config --cflags cairo igraph) -O2 -Wconversion -Wshadow -Wlogical-op -Wshift-overflow=2 -Wduplicated-cond -fstack-protector -Wstrict-prototypes

RELEASE_CFLAGS:=$(CFLAGS) -DRELEASE_MODE


build-test: $(OBJECTS) | test
	$(CC) $(CFLAGS) $^ -o build/dimagram  $(LDFLAGS) -fsanitize=address,undefined

build-release: CFLAGS=$(RELEASE_CFLAGS)
build-release: $(OBJECTS) | test
	$(CC) $(CFLAGS) $^ -o build/dimagram  $(LDFLAGS)


# build/dimagram: $(OBJECTS) | test
# 	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

build-folder:
	mkdir -p build

build/%.o: src/%.c $(HEADERS) | build-folder
	mkdir -p $(shell dirname "$@")
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean test build-release build-test

clean:
	rm -rfv build
	$(MAKE) -C test clean

test:
	$(MAKE) -C test

