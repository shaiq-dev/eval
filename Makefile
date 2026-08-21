CC      ?= cc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
LDLIBS  := -lm
INCLUDE := -Isrc

SANFLAGS := -O1 -g -std=c11 -Wall -Wextra -fno-omit-frame-pointer \
            -fsanitize=address,undefined -fno-sanitize-recover=all

SUITE := tests/test_eval.c

.PHONY: all test test-opt test-sanitize test-diff clean

all: eval

eval: src/main.c src/eval.h
	$(CC) $(CFLAGS) $(INCLUDE) src/main.c -o $@ $(LDLIBS)

test: test-opt test-sanitize test-diff

test-opt: suite-opt
	./suite-opt

test-sanitize: suite-sanitize
	./suite-sanitize

# Random expressions checked against python
test-diff: eval tests/difftest.py
	python3 tests/difftest.py

suite-opt: $(SUITE) src/eval.h
	$(CC) $(CFLAGS) $(INCLUDE) $(SUITE) -o $@ $(LDLIBS)

suite-sanitize: $(SUITE) src/eval.h
	$(CC) $(SANFLAGS) $(INCLUDE) $(SUITE) -o $@ $(LDLIBS)

clean:
	$(RM) eval suite-opt suite-sanitize
	$(RM) -r *.dSYM
