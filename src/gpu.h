#pragma once

#include <stdint.h>
#include "ps1/gpucmd.h"
#include "gte.h"

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

struct Model {
    const GTEVector16 *vertices;
    int numVertices;
    const Face *faces;
    int numFaces;
};

class Renderer {
public:
	void init(GP1VideoMode mode, int width, int height);
	
	void beginFrame(void);
	void endFrame(void);

	void drawRect(Rect rect, int r, int g, int b);
	void drawTexTri(TextureInfo &tex, Pos v0, Pos v1, Pos v2, Pos uv0, Pos uv1, Pos uv2); 
	void drawTexQuad(TextureInfo &tex, Pos v0, Pos v1, Pos v2, Pos v3, Pos uv0, Pos uv1, Pos uv2, Pos uv3);
	void drawTexRect(TextureInfo &tex, Pos pos);
	void drawModel(const Model *model, int tx, int ty, int tz, int rotX, int rotY, int rotZ);

private:
	bool usingSecondFrame;
	int frameCounter;
	DMAChain dmaChains[2];
	DMAChain *chain;

	DMAChain *getCurrentChain(void) {
		return &dmaChains[usingSecondFrame];
	}
	uint32_t *allocatePacket(int zIndex, int numCommands);
};

void uploadTexture(TextureInfo &info, const void *data, Rect pos);
void uploadIndexedTexture(TextureInfo &info, const void *image, const void *palette, int imageX, int imageY, int paletteX, int paletteY, int width, int height, GP0ColorDepth colorDepth);

}