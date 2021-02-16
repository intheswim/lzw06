CC=clang

CFLAGS = -Wall -Wextra -Werror -O2 -pedantic -ansi
CLIBS = -lm

all : main makelib

lzw06pack	: lzw06pack.c
		$(CC) $(CFLAGS) -c lzw06pack.c

lzw06unpack : lzw06unpack.c
		$(CC) $(CFLAGS) -c lzw06unpack.c

common: common.c
		$(CC) $(CFLAGS) -c common.c

main : lzw06pack lzw06unpack common main.c
		$(CC) $(CFLAGS) -o lzw06 main.c $(LIBS) lzw06pack.o lzw06unpack.o common.o

makelib: lzw06pack.o lzw06unpack.o common.o
		ar rcs liblzw06.a lzw06pack.o lzw06unpack.o common.o


.PHONY: clean

clean :
		-rm lzw06pack.o lzw06unpack.o common.o lzw06