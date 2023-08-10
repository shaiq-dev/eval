exec = eval.out
sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
flags = -g -fsanitize=address

$(exec): $(objects)
	gcc $(objects) $(flags) -o $(exec)

%.o: %.c include/%.h
	gcc -c $(flags) $< -o $@

clean:
	-rm src/*.o
