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
#define ORDERING_TABLE_SIZE  1024

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

struct XY {
	int32_t x,y;
};

struct Face {
    int vertices[3];     // 3 vertex indices
    uint32_t color;      // base color (used for modulation)
    uint8_t u[3];        // per-vertex U coordinates
    uint8_t v[3];        // per-vertex V coordinates
	TextureInfo texInfo;
    bool textured;       // true = textured, false = flat-shaded
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
	void drawTexTri(const TextureInfo &tex, XY v0, XY v1, XY v2, XY uv0, XY uv1, XY uv2, int zIndex, uint32_t col);
	void drawTexQuad(const TextureInfo &tex, XY v0, XY v1, XY v2, XY v3, XY uv0, XY uv1, XY uv2, XY uv3, int zIndex, uint32_t col);
	void drawTexRect(const TextureInfo &tex, XY pos);
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