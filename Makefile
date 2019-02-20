.PHONY: clean
CFLAGS+=-std=c99 -O2 -Wall -Wformat -Wformat-security -Werror=format-security -pie -fPIE -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

all: cpmod


cpmod: cpmod.c

clean:
	rm -f *~ cpmod *.o
