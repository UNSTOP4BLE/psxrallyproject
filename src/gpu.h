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
#define CHAIN_BUFFER_SIZE   8192
#define ORDERING_TABLE_SIZE  3072

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

struct Face {
    int16_t indices[4]; // indices into model->vertices[], last one is negative if its a triangle
    uint32_t color;      // base color (for modulation or flat shading)
    uint8_t u[4];        // per-vertex U texture coords
    uint8_t v[4];        // per-vertex V texture coords
    int32_t texid;       // texpage id, -1 for untextured
};

struct Rect {
	int32_t x,y,w,h;
};

struct XY {
	int32_t x,y;
};

struct [[gnu::packed]] ModelFileHeader {
    uint32_t magic;
    uint32_t numvertices, numfaces, numtex;

    inline bool isValid(void) const {
        return magic == (('M' << 24) | ('O' << 16) | ('D' << 8) | 'L');
    }
    inline const GTEVector16 *vertices(void) const {
        return reinterpret_cast<const GTEVector16 *>(this + 1);
    }
    inline const Face *faces(void) const {
        return reinterpret_cast<const Face *>(vertices() + numvertices);
    }
};

struct [[gnu::packed]] ModelFile {
	ModelFileHeader header;
    const GTEVector16 *vertices;
    const Face *faces;
};

class Renderer {
public:
	void init(GP1VideoMode mode, int width, int height);
	
	void beginFrame(void);
	void endFrame(void);

	void drawRect(Rect rect, int z, uint32_t col);
	void drawTexRect(const TextureInfo &tex, XY pos, int z, int col);
	void drawTri(XY v0, XY v1, XY v2, int z, uint32_t col);
	void drawTexTri(const TextureInfo &tex, XY v0, XY v1, XY v2, XY uv0, XY uv1, XY uv2, int z, uint32_t col);
	void drawQuad(XY v0, XY v1, XY v2, XY v3, int z, uint32_t col);
	void drawTexQuad(const TextureInfo &tex, XY v0, XY v1, XY v2, XY v3, XY uv0, XY uv1, XY uv2, XY uv3, int z, uint32_t col);
	void drawModel(const ModelFile *model, int tx, int ty, int tz, int rotX, int rotY, int rotZ, const TextureInfo &tex);

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
const ModelFile *loadModel(const uint8_t *data);

}