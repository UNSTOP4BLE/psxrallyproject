#include "gpu.hpp"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#ifdef PLATFORM_PSX
#include "ps1/registers.h"
#endif
namespace GFX {
static void waitForGP0Ready(void);
static void waitForDMADone(void);
static void waitForVSync(void);
static void sendLinkedList(const void *data);
static void sendVRAMData(const void *data, RECT<int32_t> rect);
static void clearOT(uint32_t *table, int numentries);

void Renderer::init(GP1VideoMode mode) {
	// Set the origin of the displayed framebuffer. These "magic" values,
	// derived from the GPU's internal clocks, will center the picture on most
	// displays and upscalers.
	int x = 0x760;
	int y = (mode == GP1_MODE_PAL) ? 0xa3 : 0x88;

	// horizontal (256, 320, 368, 512, 640) and vertical (240-256, 480-512) resolutions to pick
	GP1HorizontalRes hres;
	switch (SCREEN_WIDTH) {
		case 256:
			hres = GP1_HRES_256;
			break;
		case 320:
			hres = GP1_HRES_320;
			break;
		case 368:
			hres = GP1_HRES_368;
			break;
		case 512:
			hres = GP1_HRES_512;
			break;
		case 640:
			hres = GP1_HRES_640;
			break;
	}
	GP1VerticalRes vres = (SCREEN_HEIGHT > 256) ? GP1_VRES_512 : GP1_VRES_256;

	int offx = (SCREEN_WIDTH  * gp1_clockMultiplierH(hres)) / 2;
	int offy = (SCREEN_HEIGHT / gp1_clockDividerV(vres))    / 2;

	GPU_GP1 = gp1_resetGPU();
	GPU_GP1 = gp1_fbRangeH(x - offx, x + offx);
	GPU_GP1 = gp1_fbRangeV(y - offy, y + offy);
	GPU_GP1 = gp1_fbMode(
		hres,
		vres,
		mode,
		false,
		GP1_COLOR_16BPP
	);

	// Enable DMA for GPU and OTC
    DMA_DPCR |= DMA_DPCR_CH_ENABLE(DMA_GPU) | DMA_DPCR_CH_ENABLE(DMA_OTC);

    // Configure GPU to accept DMA for GP0 writes
    GPU_GP1 = gp1_dmaRequestMode(GP1_DREQ_GP0_WRITE);

    // Turn the display on (unblank)
    GPU_GP1 = gp1_dispBlank(false);
	usingsecondframe = false;
	framecounter = 0;
	setClearCol(64,64,64);
}

void Renderer::beginFrame(void) {
    auto newchain = getCurrentChain();

    // determine where new framebuffer to draw to is in vram
    int bufx = 0;
    int bufy = usingsecondframe ? SCREEN_HEIGHT : 0;

    // clear and prepare new chain
    clearOT(newchain->orderingtable, ORDERING_TABLE_SIZE);
    newchain->nextpacket = newchain->data;

    // add gpu commands to clear buffer and set drawing origin to new chain
    // z is set to (ORDERING_TABLE_SIZE - 1) so they're executed before anything else
    auto ptr = allocatePacket(ORDERING_TABLE_SIZE - 1, 7);
    ptr[0]   = gp0_texpage(0, true, false);
    ptr[1]   = gp0_xy(bufx, bufy);
    ptr[2]   = gp0_fbOffset2(bufx + SCREEN_WIDTH -  1, bufy + SCREEN_HEIGHT - 2);
    ptr[3]   = gp0_fbOrigin(bufx, bufy);
    ptr[4]   = clearcol | gp0_vramFill();
    ptr[5]   = gp0_xy(bufx, bufy);
    ptr[6]   = gp0_xy(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void Renderer::endFrame(void) {
    auto oldchain = getCurrentChain();

    // switch active chain
    usingsecondframe = !usingsecondframe;

    // determine where new framebuffer to display is in vram
    int bufx = 0;
    int bufy = usingsecondframe ? SCREEN_HEIGHT : 0;

    // display new framebuffer after vsync
    waitForGP0Ready();
    waitForVSync();
    GPU_GP1 = gp1_fbOffset(bufx, bufy);

    // terminate and start drawing current chain
    *(oldchain->nextpacket) = gp0_endTag(0);
    sendLinkedList(&(oldchain->orderingtable)[ORDERING_TABLE_SIZE - 1]);
}

static inline void addPacketData(uint32_t *ptr, int i, uint32_t p) {
	ptr[i] = p;       
}

void Renderer::drawRect(RECT<int32_t> rect, int z, uint32_t col) {
	auto ptr      = allocatePacket(z, 3);
	ptr[0]        = col | gp0_rectangle(false, false, false); 
	ptr[1]        = gp0_xy(rect.x, rect.y);       
	ptr[2]        = gp0_xy(rect.w, rect.h);  
}

void Renderer::drawTexRect(const TextureInfo &tex, XY<int32_t> pos, int z, int col) {
	auto ptr = allocatePacket(z, 5);
	ptr[0]        = gp0_texpage(tex.page, false, false);
	ptr[1]        = col | gp0_rectangle(true, true, false);
	ptr[2]        = gp0_xy(pos.x, pos.y);
	ptr[3]        = gp0_uv(tex.u, tex.v, tex.clut);
	ptr[4]        = gp0_xy(tex.w, tex.h);
}

void Renderer::drawTexQuad(const TextureInfo &tex, RECT<int32_t> pos, int z, uint32_t col) {
    auto ptr  = allocatePacket(z, 10);
	ptr[0]    = gp0_texpage(tex.page, false, false); // set texture page and CLUT
    ptr[1]    = col | gp0_shadedQuad(false, true, false);
    ptr[2]    = gp0_xy(pos.x, pos.y);
    ptr[3]    = gp0_uv(tex.u, tex.v, tex.clut);
    ptr[4]    = gp0_xy(pos.x+pos.w, pos.y);
    ptr[5]    = gp0_uv(tex.u+tex.w, tex.v, tex.page);
    ptr[6]    = gp0_xy(pos.x, pos.y+pos.h);
    ptr[7]    = gp0_uv(tex.u, tex.v+tex.h, 0);
    ptr[8]    = gp0_xy(pos.x+pos.w, pos.y+pos.h);
    ptr[9]    = gp0_uv(tex.u+tex.w, tex.v+tex.h, 0);
}

void Renderer::drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot) {
	gte_setControlReg(GTE_TRX, pos.x.value);
	gte_setControlReg(GTE_TRY, pos.y.value);
	gte_setControlReg(GTE_TRZ, pos.z.value);
	uint32_t one = GTE::ONE;
	gte_setRotationMatrix(
		one,    0,   0,
		  0,  one,   0,
		  0,    0, one
	);

	GTE::rotateCurrentMatrix(rot.x.value, rot.y.value, rot.z.value);

	int lasttexid = -1;

	for (int i = 0; i < static_cast<int>(model->header->numfaces); i++) {
		const Face *face = &model->faces[i];

		bool istriangle = false;
		if (face->indices[3] < 0) //for triangles the 4th indice is negative
			istriangle = true;

		// Apply perspective projection to the first 3 vertices. The GTE can
		// only process up to 3 vertices at a time, so we'll transform the
		// last one separately.
		gte_loadV0(&model->vertices[face->indices[0]]);
		gte_loadV1(&model->vertices[face->indices[1]]);
		gte_loadV2(&model->vertices[face->indices[2]]);
		gte_command(GTE_CMD_RTPT | GTE_SF);

		// Determine the winding order of the vertices on screen. If they
		// are ordered clockwise then the face is visible, otherwise it can
		// be skipped as it is not facing the camera.
		
		// todo fix converter to wind verts properly 
		gte_command(GTE_CMD_NCLIP);

		if (gte_getDataReg(GTE_MAC0) <= 0)
			continue;
		
		// Save the first transformed vertex (the GTE only keeps the X/Y
		// coordinates of the last 3 vertices processed and Z coordinates of
		// the last 4 vertices processed) and apply projection to the last
		// vertex.
		uint32_t xy0 = 0;
		if (!istriangle) {
			xy0 = gte_getDataReg(GTE_SXY0);
			gte_loadV0(&model->vertices[face->indices[3]]);
			gte_command(GTE_CMD_RTPS | GTE_SF);
			
			// Calculate the average Z coordinate of all vertices and use it to
			// determine the ordering table bucket index for this face.
			gte_command(GTE_CMD_AVSZ4 | GTE_SF);
		}
		else
			gte_command(GTE_CMD_AVSZ3 | GTE_SF);

		int z = gte_getDataReg(GTE_OTZ);

		if ((z < 0) || (z >= ORDERING_TABLE_SIZE))
			continue;

		uint32_t *ptr;
		if (face->texid >= 0) { //textured
			auto clut = model->textures[face->texid].clut;
			auto page = model->textures[face->texid].page;
			
			if (istriangle) {
				ptr           = allocatePacket(z, 7);
				ptr[0]        = face->color | gp0_shadedTriangle(false, true, false);
				gte_storeDataReg(GTE_SXY0, 1 * 4, ptr);
				ptr[2]        = gp0_uv(face->u[0], face->v[0], clut);
				gte_storeDataReg(GTE_SXY1, 3 * 4, ptr);
				ptr[4]        = gp0_uv(face->u[1], face->v[1], page);
				gte_storeDataReg(GTE_SXY2, 5 * 4, ptr);
				ptr[6]        = gp0_uv(face->u[2], face->v[2], 0);
			}
			else { //quad
	    		ptr       = allocatePacket(z, 9);
				ptr[0]    = face->color | gp0_shadedQuad(false, true, false);
				ptr[1]    = xy0;
				ptr[2]    = gp0_uv(face->u[0], face->v[0], clut);
				gte_storeDataReg(GTE_SXY0, 3 * 4, ptr);
				ptr[4]    = gp0_uv(face->u[1], face->v[1], page);
				gte_storeDataReg(GTE_SXY1, 5 * 4, ptr);
				ptr[6]    = gp0_uv(face->u[2], face->v[2], 0);
				gte_storeDataReg(GTE_SXY2, 7 * 4, ptr);
				ptr[8]    = gp0_uv(face->u[3], face->v[3], 0);
			}	

			if (lasttexid != face->texid) {
	    		ptr       = allocatePacket(z, 1);
				ptr[0]    = gp0_texpage(page, false, false); // set texture page and CLUT		
				lasttexid = face->texid;
			}
		}
		else  { //untextured
			if (istriangle) {
				ptr    = allocatePacket(z,4);
				ptr[0] = face->color | gp0_shadedTriangle(false, false, false);
				gte_storeDataReg(GTE_SXY0, 1 * 4, ptr);
				gte_storeDataReg(GTE_SXY1, 2 * 4, ptr);
				gte_storeDataReg(GTE_SXY2, 3 * 4, ptr);
			}
			else { //quad
	    		ptr    = allocatePacket(z, 5);
				ptr[0] = face->color | gp0_shadedQuad(false, false, false);
				ptr[1] = xy0;
				gte_storeDataReg(GTE_SXY0, 2 * 4, ptr);
				gte_storeDataReg(GTE_SXY1, 3 * 4, ptr);
				gte_storeDataReg(GTE_SXY2, 4 * 4, ptr);
			}
		}
	}
}

uint32_t *Renderer::allocatePacket(int z, int numcommands) {
    auto chain = getCurrentChain();
    auto ptr   = chain->nextpacket;

    // check z index is valid
    assert((z >= 0) && (z < ORDERING_TABLE_SIZE));

    // link new packet into ordering table at specified z index
    *ptr = gp0_tag(numcommands, (void *) chain->orderingtable[z]);
    chain->orderingtable[z] = gp0_tag(0, ptr);

    // bump up allocator and check we haven't run out of space
    chain->nextpacket += numcommands + 1;
    assert(chain->nextpacket < &(chain->data)[CHAIN_BUFFER_SIZE]);

    return &ptr[1];
}

void Renderer::printString(XY<int32_t> pos, int z, const char *str) {
	assert(fontmap);
	int curx = pos.x, cury = pos.y;

	uint32_t *ptr;
	int tabwidth = fontmap->header->tabwidth;

	for (; *str; str++) {
		char c = *str;

		// Check if the character is "special"
		switch (c) {
			case '\t':
				curx += tabwidth - 1;
				curx -= curx % tabwidth;
				continue;

			case '\n':
				curx  = pos.x;
				cury += fontmap->header->lineheight;
				continue;

			case ' ':
				curx += fontmap->header->spacewidth;
				continue;

			case '\x80' ... '\xff':
				c = '\x7f';
				break;
		}

		const RECT<uint8_t> &rect = fontmap->rects[c - fontmap->header->firstchar];
		
		// Enable blending to make sure any semitransparent pixels in the font get rendered correctly.
		ptr    = allocatePacket(z, 4);
		ptr[0] = gp0_rectangle(true, true, true);
		ptr[1] = gp0_xy(curx, cury);
		ptr[2] = gp0_uv(fonttex.u + rect.x, fonttex.v + rect.y, fonttex.clut);
		ptr[3] = gp0_xy(rect.w, rect.h);

		curx += rect.w;
	}
	
	ptr    = allocatePacket(z, 1);
	ptr[0] = gp0_texpage(fonttex.page, false, false);
}

void Renderer::printStringf(XY<int32_t> pos, int z, const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    printString(pos, z, buf);
}

//indexed
void uploadTexture(TextureInfo &info, const void *image) {
    const TexHeader* header = reinterpret_cast<const TexHeader*>(image);
	assert(header->isValid());
	info = header->texinfo;

	assert((info.w <= 256) && (info.h <= 256));

	int ncolors = (info.bpp == 8) ? 256 : 16;
	int widthdivider = (info.bpp == 8) ? 2 : 4;
	
	sendVRAMData(header->texdata(), {header->vrampos[0], header->vrampos[1], info.w / widthdivider, info.h});
	waitForDMADone();
	sendVRAMData(header->clut(), {header->clutpos[0], header->clutpos[1], ncolors, 1});
	waitForDMADone();
}

//todo add freeing functions for font
Model* loadModel(const uint8_t* data) {
    auto model = new Model(); 
    model->header = reinterpret_cast<const ModelFileHeader*>(data);
	assert(model->header->isValid());

    model->vertices = model->header->vertices();
    model->faces = model->header->faces();

	//textures
	auto texptr = model->header->textures();
	
	int ntex = static_cast<int>(model->header->numtex);

    if (ntex > 0) {
		model->textures = new TextureInfo[ntex];

		for (int i = 0; i < static_cast<int>(model->header->numtex); i++) {
			const TexHeader* tex_header = reinterpret_cast<const TexHeader*>(texptr);
			uploadTexture(model->textures[i], texptr);
			
			size_t clutbytes = tex_header->clutsize;
			size_t texbytes = tex_header->texsize;

			texptr += sizeof(TexHeader) + clutbytes + texbytes;
		}
	}
    return model;
}

void freeModel(Model *model) {
    // free textures array if allocated
    if (model->textures) {
        delete[] model->textures;
    }

    // free the model itself
    delete model;
}

//todo proper font manager for multiple fonts
FontData *loadFontMap(const uint8_t *data) {
    auto fntdata = new FontData();
    fntdata->header = reinterpret_cast<const FontHeader*>(data);
	assert(fntdata->header->isValid());
    fntdata->rects = fntdata->header->rects();

	return fntdata;
}

static void waitForGP0Ready(void) {
	while (!(GPU_GP1 & GP1_STAT_CMD_READY))
		__asm__ volatile("");
}

static void waitForDMADone(void) {
	while (DMA_CHCR(DMA_GPU) & DMA_CHCR_ENABLE)
		__asm__ volatile("");
}

static void waitForVSync(void) {
	while (!(IRQ_STAT & (1 << IRQ_VSYNC)))
		__asm__ volatile("");

	IRQ_STAT = ~(1 << IRQ_VSYNC);
}

static void sendLinkedList(const void *data) {
	waitForDMADone();
	assert(!((uint32_t) data % 4));

	DMA_MADR(DMA_GPU) = (uint32_t) data;
	DMA_CHCR(DMA_GPU) = 0
		| DMA_CHCR_WRITE
		| DMA_CHCR_MODE_LIST
		| DMA_CHCR_ENABLE;
}

static void sendVRAMData(const void *data, RECT<int32_t> rect) {
	waitForDMADone();
	assert(!((uint32_t) data % 4));

	size_t length = (rect.w * rect.h) / 2;
	size_t chunksize, numchunks;

	if (length < DMA_MAX_CHUNK_SIZE) {
		chunksize = length;
		numchunks = 1;
	} else {
		chunksize = DMA_MAX_CHUNK_SIZE;
		numchunks = length / DMA_MAX_CHUNK_SIZE;

		assert(!(length % DMA_MAX_CHUNK_SIZE));
	}

	waitForGP0Ready();
	GPU_GP0 = gp0_vramWrite();
	GPU_GP0 = gp0_xy(rect.x, rect.y);
	GPU_GP0 = gp0_xy(rect.w, rect.h);

	DMA_MADR(DMA_GPU) = (uint32_t) data;
	DMA_BCR (DMA_GPU) = chunksize | (numchunks << 16);
	DMA_CHCR(DMA_GPU) = 0
		| DMA_CHCR_WRITE
		| DMA_CHCR_MODE_SLICE
		| DMA_CHCR_ENABLE;
}

static void clearOT(uint32_t *table, int numentries) {
	DMA_MADR(DMA_OTC) = (uint32_t) &table[numentries - 1];
	DMA_BCR (DMA_OTC) = numentries;
	DMA_CHCR(DMA_OTC) = 0
		| DMA_CHCR_READ
		| DMA_CHCR_REVERSE
		| DMA_CHCR_MODE_BURST
		| DMA_CHCR_ENABLE
		| DMA_CHCR_TRIGGER;

	while (DMA_CHCR(DMA_OTC) & DMA_CHCR_ENABLE)
		__asm__ volatile("");
}
}