CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror

all: build

build:
	gcc $(CFLAGS) main.c -o myls -lm


run: build
	./myls

valgrind: build
	valgrind --leak-check=full ./myls

clean:
	rm -f myls
