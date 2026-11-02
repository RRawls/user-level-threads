CC      = gcc
CFLAGS  = -Wall -g -O1 -I.
TESTS   = $(basename $(notdir $(wildcard tests/test_*.c)))

libworker.a: thread-worker.o
	ar rcs $@ $^

build/%: tests/%.c libworker.a
	@mkdir -p build
	$(CC) $(CFLAGS) $< -L. -lworker -o $@

test: $(addprefix build/,$(TESTS))
	@for t in $(TESTS); do timeout 60 build/$$t || exit 1; done

clean:
	rm -rf build *.o *.a