#pragma once

#include "ps1/gpucmd.h"
#include "gte.h"

// In order for Z averaging to work properly, ORDERING_TABLE_SIZE should be set
// to either a relatively high value (1024 or more) or a multiple of 12; see
// setupGTE() for more details. Higher values will take up more memory but are
// required to render more complex scenes with wide depth ranges correctly.
namespace GFX {

#define DMA_MAX_CHUNK_SIZE    16
#define CHAIN_BUFFER_SIZE   4104
#define ORDERING_TABLE_SIZE  3072

constexpr int SCREEN_WIDTH  = 320;
constexpr int SCREEN_HEIGHT = 240;

struct DMAChain {
	uint32_t data[CHAIN_BUFFER_SIZE];
	uint32_t orderingtable[ORDERING_TABLE_SIZE];
	uint32_t *nextpacket;
};

struct Face {
    int16_t indices[4];  // indices into model->vertices[], last one is negative if its a triangle
    uint32_t color;      // base color (for modulation or flat shading)
    uint8_t u[4];        // per-vertex U texture coords
    uint8_t v[4];        // per-vertex V texture coords
    int32_t texid;       // texpage id, -1 for untextured
};

template<typename T>
struct [[gnu::packed]] RECT {
    T x, y, w, h;

    RECT() = default;
    RECT(T _x, T _y, T _w, T _h) : x(_x), y(_y), w(_w), h(_h) {}
};

template<typename T>
struct [[gnu::packed]] XY {
    T x, y;

    XY() = default;
    XY(T _x, T _y) : x(_x), y(_y) {}
};

struct [[gnu::packed]] TextureInfo {
	uint8_t  u, v;
	uint16_t w, h;
	uint16_t page, clut;
	uint16_t bpp;
};

struct [[gnu::packed]] TexHeader {
	uint32_t magic;
	TextureInfo texinfo;
	uint16_t vrampos[2];
	uint16_t clutpos[2];
	uint16_t clutsize;
	uint16_t texsize;

    inline bool isValid(void) const {
        return magic == ('X' | ('T' << 8) | ('E' << 16) | ('X' << 24));
    }
    inline const uint16_t *clut(void) const {
        return reinterpret_cast<const uint16_t *>(this + 1);
    }
    inline const uint8_t *texdata(void) const {
        return reinterpret_cast<const uint8_t *>(clut() + (clutsize / sizeof(uint16_t)));
    }
};

struct [[gnu::packed]] ModelFileHeader {
    uint32_t magic;
    uint32_t numvertices, numfaces, numtex;

    inline bool isValid(void) const {
        return magic == ('X' | ('M' << 8) | ('D' << 16) | ('L' << 24));
    }
    inline const GTEVector16 *vertices(void) const {
        return reinterpret_cast<const GTEVector16 *>(this + 1);
    }
    inline const Face *faces(void) const {
        return reinterpret_cast<const Face *>(vertices() + numvertices);
    }
	inline const uint8_t *textures(void) const {
	    return reinterpret_cast<const uint8_t *>(faces() + numfaces);
	}
};

struct [[gnu::packed]] ModelFile {
	const ModelFileHeader *header;
    const GTEVector16 *vertices;
    const Face *faces;
	TextureInfo *textures;
};

struct [[gnu::packed]] FontHeader {
    uint32_t magic; 
    uint8_t firstchar; 
    uint8_t spacewidth; 
    uint8_t tabwidth; 
    uint8_t lineheight; 
    uint8_t numchars; 
    uint8_t _padding[3]; 

    inline bool isValid(void) const {
        return magic == ('X' | ('F' << 8) | ('N' << 16) | ('T' << 24));
    }
    inline const GFX::RECT<uint8_t> *rects(void) const {
        return reinterpret_cast<const GFX::RECT<uint8_t> *>(this + 1);
    }
};

struct [[gnu::packed]] FontData {
    const FontHeader *header;
    const GFX::RECT<uint8_t> *rects; 
};

class Renderer {
public:
	void init(GP1VideoMode mode);
	
	void beginFrame(void);
	void endFrame(void);

	void drawRect(RECT<int32_t> rect, int z, uint32_t col);
	void drawTexRect(const TextureInfo &tex, XY<int32_t> pos, int z, int col);
    void drawTexQuad(const TextureInfo &tex, RECT<int32_t> pos, int z, uint32_t col);
	void drawModel(const ModelFile *model, int tx, int ty, int tz, int rotX, int rotY, int rotZ); //todo dont use random ints as args
	void printString(XY<int32_t> pos, int zIndex, const char *str);

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
const ModelFile *loadModel(const uint8_t *data);
FontData *loadFontMap(const uint8_t *data);

}