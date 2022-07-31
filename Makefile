CFLAGS=-std=c11 -g -fno-common
CC=gcc

# get all c files
SRC=$(wildcard *.c)
# *.c => *.o
OBJ=$(SRC:.c=.o)

# e.g. chibicc-wyj: main.o tokenize.o parse.o codegen.o
chibicc-wyj: ${OBJ}
	@#$(CC) ${CFLAGS} -o $@ $? $(LDFLAGS)
	$(CC) ${CFLAGS} -o $@ ${OBJ} $(LDFLAGS)

# all C files depend the header file
${OBJ}: chibicc_wyj.h

test: chibicc-wyj
	./test.sh

clean:
	rm -f chibicc-wyj *.o tmp*

.PHONY: test clean
