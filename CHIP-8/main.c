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
SDL_Color background = {0, 0, 0, 255};
SDL_Color foreground = {255, 255, 255, 255};
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
	
	// Make sure pixel graphics scale appropriately
	SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, 0);
	
	SDL_RenderSetLogicalSize(renderer, 64, 32);
	SDL_RenderSetIntegerScale(renderer, 1);
}

void quitSDL(void){
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
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
	fread(&chip8->memory[0x200], romFileSize, 1, romFile);
	
	fclose(romFile);
	romFile = NULL;
}

void unrecognisedOpcode(uint16_t opcode){
	fprintf(stderr, "Opcode 0x%04x unrecognised\n", opcode);
	exit(EXIT_FAILURE);
}

static void draw(Chip8 *chip8, SDL_Surface *surface) {
	SDL_LockSurface(surface);
	SDL_memcpy(surface->pixels, chip8->display, sizeof(chip8->display));
	SDL_UnlockSurface(surface);
	
	SDL_RenderClear(renderer);
	SDL_Texture * texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	SDL_DestroyTexture(texture);
	texture = NULL;
}

// Instructions
void x0(Chip8* chip8){
	switch (chip8->opcode) {
			// Clear screen
		case 0x00E0:
			memset(chip8->display, 0, sizeof(chip8->display));
			break;
			
		default:
			unrecognisedOpcode(chip8->opcode);
			break;
	}
}

void x1(Chip8* chip8){
	// 1NNN - Jump to address NNN
	chip8->pc = (chip8->opcode & 0x0FFF);
}

void x6(Chip8* chip8){
	// 6XNN - Set register VX to value NN
	chip8->V[((chip8->opcode & 0x0F00) >> 8)] = (chip8->opcode & 0x00FF);
}

void x7(Chip8* chip8){
	// 7XNN - Add value NN to register VX
	chip8->V[((chip8->opcode & 0x0F00) >> 8)] += (chip8->opcode & 0x00FF);
}

void xA(Chip8* chip8){
	// ANNN - Set Index register to address NNN
	chip8->index = (chip8->opcode & 0x0FFF);
}

void xD(Chip8* chip8){
	// DXYN - Draw a sprite N pixels tall from the index register at position
	// VX, VY
	int8_t X = chip8->V[(chip8->opcode & 0x0F00) >> 8] % 64;
	int8_t Y = chip8->V[(chip8->opcode & 0x00F0) >> 4] % 32;
	int8_t N = chip8->opcode & 0x000F;
	
	// Set VF to 0 to reset flag
	chip8->V[0xF] = 0;
	
	for (int row = 0; row < N; row++) {
		int8_t pixelByte = chip8->memory[chip8->index + row];
		
		// Break if we reach the bottom of the screen
		if (Y + row > 31) {
			break;
		}
		
		for (int col = 0; col < 8; col++) {
			// Break if we reach the right edge of the screen
			if (X + col > 63) {
				break;
			}
			
			// Read the bit corresponding to the current position in the
			// sprite we're drawing
			bool pixel = (pixelByte & (0x80 >> col)) >> (7 - col);
			bool* displayPixel = &chip8->display[(X + col) + ((Y + row) * 64)];
			// If bit is already being displayed then turn it on and set flag
			if (pixel && *displayPixel) {
				*displayPixel = false;
				chip8->V[0xF] = 1;
			// If bit is not being displayed, turn it on
			} else if (pixel && !*displayPixel){
				*displayPixel = true;
			}
		}
	}
}

void cpuCycle(Chip8* chip8){
	// Fetch
	chip8->opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc + 1];
	chip8->pc += 2;
	
	// Decode
	int8_t importantNibble = (chip8->opcode & 0xF000) >> 12;
	
	// Execute
	switch (importantNibble) {
		case 0x0:
			x0(chip8);
			break;
		case 0x1:
			x1(chip8);
			break;
		case 0x6:
			x6(chip8);
			break;
		case 0x7:
			x7(chip8);
			break;
		case 0xA:
			xA(chip8);
			break;
		case 0xD:
			xD(chip8);
			break;
		default:
			unrecognisedOpcode(chip8->opcode);
			break;
	}
}

void loop(Chip8* chip8){
	bool running = true;
	SDL_Event event;
	
	SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE,
														   64, 32, 1, SDL_PIXELFORMAT_INDEX8);
	SDL_Color colors[2] = {background, foreground};
	SDL_SetPaletteColors(surface->format->palette, colors, 0, 2);
	
	while (running) {
		
		Uint64 start = SDL_GetPerformanceCounter();
		
		// Determine how many times to cycle the cpu in order to match the
		// desired clock speed at a refresh rate of 60fps
		for(int i = 0; i < (clockSpeed/60); i++){
			cpuCycle(chip8);
		}
		
		draw(chip8, surface);
		
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT) {
			running = false;
		}
		
		Uint64 end = SDL_GetPerformanceCounter();
		float elapsed = (end - start) / (float) SDL_GetPerformanceFrequency();
		float elapsedMS = elapsed * 1000.0f;
		// Wait until it's time for the next frame
		SDL_Delay(floor(16.666f - elapsedMS));
	}
	
	SDL_FreeSurface(surface);
	surface = NULL;
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
