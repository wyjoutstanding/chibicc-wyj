CFLAGS=-std=c11 -g -fno-common

chibicc-wyj: main.o
	$(CC) -o chibicc-wyj main.o $(LDFLAGS)


test: chibicc-wyj
	./test.sh

clean:
	rm -f chibicc-wyj *.o tmp*

.PHONY: test clean
