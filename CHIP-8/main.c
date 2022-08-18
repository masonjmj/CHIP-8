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
	bool display[64*32];  // Pixel array for display
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
SDL_Renderer* renderer = NULL;
int scale = 10;
int clockSpeed = 500;

void initializeSDL(void){
	if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 )
	{
		printf("Error : %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	window = SDL_CreateWindow("CHIP-8",
							  SDL_WINDOWPOS_UNDEFINED,
							  SDL_WINDOWPOS_UNDEFINED,
							  64*scale, 32*scale, 0);
	
	if (!window) {
		printf("Error : %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!window) {
		printf("Error : %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, 0);
	
	SDL_RenderSetLogicalSize(renderer, 64, 32);
	SDL_RenderSetIntegerScale(renderer, 1);
}

void quitSDL(void){
	SDL_DestroyWindow(window);
	SDL_Quit();
	window = NULL;
}

void setupCHIP(Chip8* chip8){
	// Clear all data to make sure everything is 0
	memset(chip8, 0, sizeof(*chip8));
	
	uint8_t fontData[] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
	
	// Load font data into the start of memory
	memcpy(chip8->memory, &fontData, sizeof(fontData));
	
	// Start program counter at 0x200 for compatibility
	chip8->pc = 0x200;
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
	
	// Read contents of the file into memory starting at address 0x200
	fread(&chip8->memory[0x1FF], romFileSize, 1, romFile);
	
	fclose(romFile);
	romFile = NULL;
}

inline void unrecognisedOpcode(uint16_t opcode){
	fprintf(stderr, "Opcode %x unrecognised\n", opcode);
}

static void draw(Chip8 *chip8, SDL_Surface *surface) {
	SDL_LockSurface(surface);
	SDL_memcpy(surface->pixels, chip8->display, sizeof(chip8->display));
	SDL_UnlockSurface(surface);
	
	SDL_RenderClear(renderer);
	SDL_Texture * screen_texture = SDL_CreateTextureFromSurface(renderer,
																surface);
	SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	SDL_DestroyTexture(screen_texture);
}

void loop(Chip8* chip8){
	bool running = true;
	SDL_Event event;
	
	SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE,
	   64, 32, 1, SDL_PIXELFORMAT_INDEX8);
	SDL_Color colors[2] = {{0, 0, 0, 255}, {255, 255, 255, 255}};
	SDL_SetPaletteColors(surface->format->palette, colors, 0, 2);
	
	
	while (running) {
		
		draw(chip8, surface);
		
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT) {
			running = false;
		}
	}
	
	SDL_FreeSurface(surface);
}

static void handleOptions(int argc, char *const *argv, char **filePath) {
	static struct option long_options[] = {
		{"scale", required_argument, NULL, 's'},
		{"clock", required_argument, NULL, 'c'},
		{NULL, 0, NULL, 0}
	};
	
	int c;
	while ((c = getopt_long(argc, argv, "s:c:", long_options, NULL)) != -1) {
		switch (c) {
			case 's':
				if (atoi(optarg) != 0) {
					scale = atoi(optarg);
				} else {
					fprintf(stderr, "Scale must be a non-zero integer\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'c':
				if (atoi(optarg) != 0) {
					clockSpeed = atoi(optarg);
				} else {
					fprintf(stderr, "Clock must be a non-zero integer\n");
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
		*filePath = argv[optind];
	} else {
		fprintf(stderr, "You must specify the path to the ROM you wish to load\n");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char * const argv[]) {
	
	char* filePath;

	handleOptions(argc, argv, &filePath);
	
	Chip8 chip8;
	setupCHIP(&chip8);
	loadROM(filePath, &chip8);
	
	for (int i = 0; i < (64*32); i++) {
		if (i % 7 == 0) {
			chip8.display[i] = true;
		}
	}
	initializeSDL();
	loop(&chip8);
	quitSDL();

	
//	for (int i = 0; i < sizeof(chip8.memory); i++) {
//		printf("%d %x\n", i, chip8.memory[i]);
//	}
//
	
	return EXIT_SUCCESS;
}
