#pragma once

#include "../gpu.hpp"
#include "ps1/gpucmd.h"
#include "../gte.hpp"
#include "../fixed.hpp"

// In order for Z averaging to work properly, ORDERING_TABLE_SIZE should be set
// to either a relatively high value (1024 or more) or a multiple of 12; see
// setupGTE() for more details. Higher values will take up more memory but are
// required to render more complex scenes with wide depth ranges correctly.
namespace GFX {

#define DMA_MAX_CHUNK_SIZE    16
#define CHAIN_BUFFER_SIZE   4104
#define ORDERING_TABLE_SIZE  1024

struct DMAChain {
	uint32_t data[CHAIN_BUFFER_SIZE];
	uint32_t orderingtable[ORDERING_TABLE_SIZE];
	uint32_t *nextpacket;
};

class PSXRenderer : public Renderer {
public:
	void init(int mode);
	
	void beginFrame(void);
	void endFrame(void);

	void drawRect(RECT<int32_t> rect, int z, uint32_t col);
	void drawTexRect(const TextureInfo &tex, XY<int32_t> pos, int z, int col);
    void drawTexQuad(const TextureInfo &tex, RECT<int32_t> pos, int z, uint32_t col);
	void drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot);
	void printString(XY<int32_t> pos, int z, const char *str);
    void printStringf(XY<int32_t> pos, int z, const char *fmt, ...);

    void setClearCol(uint8_t r, uint8_t g, uint8_t b) {
        clearcol = gp0_rgb(r, g, b);
    }
	TextureInfo fonttex;
	FontData *fontmap;
private:
	bool usingsecondframe;
	int framecounter;
	DMAChain dmachains[2];
    uint32_t clearcol;

	DMAChain *getCurrentChain(void) {
		return &dmachains[usingsecondframe];
	}

	uint32_t *allocatePacket(int z, int numcommands);
};

void uploadTexture(TextureInfo &info, const void *image);
Model *loadModel(const uint8_t *data);
void freeModel(Model *model);
FontData *loadFontMap(const uint8_t *data);

}