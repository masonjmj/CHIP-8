make: main.c
	mkdir -p bin
	clang main.c -lSDL2 -lm -o bin/chip8
