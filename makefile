CC       = gcc
CPPFLAGS =
CFLAGS   = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -O2
LINKOPTIONS = -shared -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup

.PHONY: all clean

nand.o: nand.c nand.h
memory_tests.o: memory_tests.c memory_tests.h

libnand.so: nand.o memory_tests.o
	gcc $(LINKOPTIONS) -o $@ $^

clean:
	rm -rf *.a *.so *.o