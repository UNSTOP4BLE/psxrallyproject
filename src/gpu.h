#pragma once

#include <stdint.h>
#include "ps1/gpucmd.h"

// In order for Z averaging to work properly, ORDERING_TABLE_SIZE should be set
// to either a relatively high value (1024 or more) or a multiple of 12; see
// setupGTE() for more details. Higher values will take up more memory but are
// required to render more complex scenes with wide depth ranges correctly.
namespace GFX {

#define DMA_MAX_CHUNK_SIZE    16
#define CHAIN_BUFFER_SIZE   1024
#define ORDERING_TABLE_SIZE  240

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240


struct DMAChain {
	uint32_t data[CHAIN_BUFFER_SIZE];
	uint32_t orderingTable[ORDERING_TABLE_SIZE];
	uint32_t *nextPacket;
};

struct TextureInfo {
	uint8_t  u, v;
	uint16_t width, height;
	uint16_t page, clut;
};

struct Rect {
	int32_t x,y,w,h;
};

struct Pos {
	int32_t x,y;
};

class Renderer {
public:
	void init(GP1VideoMode mode, int width, int height);
	
	void clear(void);
	void flip(void);
	void drawRect(Rect rect, int r, int g, int b);
	void drawTexRect(TextureInfo &tex, Pos pos);
private:
	bool usingSecondFrame = false;
	int frameCounter = 0;
	DMAChain dmaChains[2];
	DMAChain *chain;
};

void uploadTexture(TextureInfo &info, const void *data, Rect pos);
void uploadIndexedTexture(TextureInfo &info, const void *image, const void *palette, int imageX, int imageY, int paletteX, int paletteY, int width, int height, GP0ColorDepth colorDepth);

}