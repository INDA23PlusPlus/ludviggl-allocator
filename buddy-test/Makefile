
libbuddy.so: ../buddy.h buddy.c
	gcc -fPIC \
		-Wall -Wextra -Wpedantic \
		-g -O0\
		-DBUDDY_STDLIB_OVERRIDE \
		-shared \
		-o libbuddy.so \
		buddy.c

.PHONY: clean

clean:
	rm -f *.o *.so*
