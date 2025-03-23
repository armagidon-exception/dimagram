SOURCES=$(shell find ./ -type f -name '*_test.c')
# OBJECTS=$(patsubst ./%,build/%, $(SOURCES:.c=.o))

UTILS=$(shell find ./ -type f ! -name '*_test.c' -name '*.c')
UTILS_OBJECTS=$(patsubst ./%,build/%,$(UTILS:.c=.o))

TARGETS=$(shell find ../src/ -name '*.c' ! -name 'main.c')
TARGET_OBJECTS=$(patsubst ./src/%,build/%,$(TARGETS:.c=.o))

TESTS=$(patsubst ./%,build/%, $(SOURCES:.c=))

LDFLAGS=-lm $(shell pkg-config --libs igraph cairo libunwind-ptrace) -ldl
CFLAGS=\
			-Wall \
			-Wextra \
			-ggdb \
			-pedantic \
			-rdynamic \
			-std=gnu11 \
			-I../src \
			$(shell pkg-config --cflags igraph cairo) \
			-Og \
			-Wconversion \
			-Wshadow \
			-Wlogical-op \
			-Wshift-overflow=2 \
			-Wduplicated-cond \
			-fstack-protector \
			-Wstrict-prototypes \
			-DTESTING_ENV

test-all: $(TESTS) | build-folder
	for test in $(TESTS); do $$test; done

build/%.o: %.c | build-folder
	$(CC) $(CFLAGS) -c $^ -o $@ $(LDFLAGS)

build/%: %.c $(TARGET_OBJECTS) $(UTILS_OBJECTS) | build-folder
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

build-folder:
	mkdir -p build

.PHONY: clean test-all

clean:
	rm -rfv build
