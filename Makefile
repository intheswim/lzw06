CC=clang
GCC=clang++

CFLAGS = -Wall -Wextra -Werror -O2 -pedantic -ansi
CPPFLAGS = -Wall -Wextra -O2
CLIBS = -lm

all : main makelib libtest

lzw06pack	: lzw06pack.c
		$(CC) $(CFLAGS) -c lzw06pack.c

lzw06unpack : lzw06unpack.c
		$(CC) $(CFLAGS) -c lzw06unpack.c

common: common.c
		$(CC) $(CFLAGS) -c common.c

main : lzw06pack lzw06unpack common main.c
		$(CC) $(CFLAGS) -o lzw06 main.c $(CLIBS) lzw06pack.o lzw06unpack.o common.o

makelib: lzw06pack.o lzw06unpack.o common.o
		ar rcs liblzw06.a lzw06pack.o lzw06unpack.o common.o

libtest : libtest.cpp
		$(GCC) $(CPPFLAGS) -o lzw_test libtest.cpp $(CLIBS) -L. -llzw06


.PHONY: clean

clean :
		-rm lzw06pack.o lzw06unpack.o common.o lzw06 liblzw06.a lzw_test