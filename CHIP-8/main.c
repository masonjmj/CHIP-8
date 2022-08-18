//
//  main.c
//  CHIP-8
//
//  Created by James Mason on 2022-08-17.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <SDL2/SDL.h>

typedef struct Chip8 {
	uint8_t memory[4096];  // 4kB RAM
	uint8_t display[64*32];  // Pixel array for display
	uint8_t keypad[16];  // 16 digit keypad
	uint8_t V[16];  // Registers 0-15
	uint16_t stack[16];  // Stack of addresses for returning from calls
	uint8_t sp;  // Stack pointer
	uint8_t delay;  // Delay timer
	uint8_t sound;  // Sound timer
	uint16_t pc;  // Program counter
	uint16_t index;  // Index register
	uint16_t opcode;  // Current instruction
} Chip8;

// Globals
SDL_Window* window = NULL;
//int scale = 10;

void initializeSDL(){
	
}

void setupCHIP(Chip8* chip8){
	// Clear all data to make sure everything is 0
	memset(chip8, 0, sizeof(*chip8));
	
	
}

void loadROM(char* filePath, Chip8* chip8){
	FILE* romFile = fopen(filePath, "rb");
	if (romFile == NULL) {
		fprintf(stderr, "Could not open %s", filePath);
		exit(EXIT_FAILURE);
	}
	
	fseek(romFile, 0, SEEK_END);
	size_t romFileSize = ftell(romFile);
	fseek(romFile, 0, SEEK_SET);
	
	if (romFileSize > sizeof(chip8->memory) - 512) {
		fprintf(stderr, "The size of the specified ROM is larger than the available memory");
		exit(EXIT_FAILURE);
	}
	
	fread(&chip8->memory[0x1FF], romFileSize, 1, romFile);
}

inline void unrecognisedOpcode(uint16_t opcode){
	fprintf(stderr, "Opcode %x unrecognised\n", opcode);
}

static void handleOptions(int argc, char *const *argv,
						  char **filePath, int *scale) {
	static struct option long_options[] = {
		{"scale", required_argument, NULL, 's'},
		{NULL, 0, NULL, 0}
	};
	
	int c;
	while ((c = getopt_long(argc, argv, "s:", long_options, NULL)) != -1) {
		switch (c) {
			case 's':
				if (atoi(optarg) != 0) {
					*scale = atoi(optarg);
				} else {
					fprintf(stderr, "Scale must be a non-zero integer\n");
					exit(EXIT_FAILURE);
				}
				break;
			case '?':
			default:
				exit(EXIT_FAILURE);
				break;
		}
	}
	// Get input file-path
	if (optind < argc) {
		printf("%s\n", argv[optind]);
		*filePath = argv[optind];
	} else {
		fprintf(stderr, "You must specify the path to the ROM you wish to load\n");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char * const argv[]) {
	
	char* filePath;
	int scale = 10;

	handleOptions(argc, argv, &filePath, &scale);
	
	Chip8 chip8;
	setupCHIP(&chip8);
	loadROM(filePath, &chip8);
	
	
	for (int i = 0; i < sizeof(chip8.memory); i++) {
		printf("%d %x\n", i, chip8.memory[i]);
	}
	
	printf("Scale is %d\n", scale);
	
	return EXIT_SUCCESS;
}
