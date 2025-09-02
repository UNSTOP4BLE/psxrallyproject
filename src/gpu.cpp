#include "gpu.h"
#include <assert.h>
#include <stdint.h>
#include "ps1/registers.h"

namespace GFX {
static void waitForGP0Ready(void);
static void waitForDMADone(void);
static void waitForVSync(void);
static void sendLinkedList(const void *data);
static void sendVRAMData(const void *data, RECT<int32_t> rect);
static void clearOrderingTable(uint32_t *table, int numEntries);

void Renderer::init(GP1VideoMode mode, int width, int height) {
	// Set the origin of the displayed framebuffer. These "magic" values,
	// derived from the GPU's internal clocks, will center the picture on most
	// displays and upscalers.
	int x = 0x760;
	int y = (mode == GP1_MODE_PAL) ? 0xa3 : 0x88;

	// Set the resolution. The GPU provides a number of fixed horizontal (256,
	// 320, 368, 512, 640) and vertical (240-256, 480-512) resolutions to pick
	// from, which affect how fast pixels are output and thus how "stretched"
	// the framebuffer will appear.
	GP1HorizontalRes horizontalRes = GP1_HRES_320;
	GP1VerticalRes   verticalRes   = GP1_VRES_256;

	// Set the number of displayed rows and columns. These values are in GPU
	// clock units rather than pixels, thus they are dependent on the selected
	// resolution.
	int offsetX = (width  * gp1_clockMultiplierH(horizontalRes)) / 2;
	int offsetY = (height / gp1_clockDividerV(verticalRes))      / 2;

	// Hand all parameters over to the GPU by sending GP1 commands.
	GPU_GP1 = gp1_resetGPU();
	GPU_GP1 = gp1_fbRangeH(x - offsetX, x + offsetX);
	GPU_GP1 = gp1_fbRangeV(y - offsetY, y + offsetY);
	GPU_GP1 = gp1_fbMode(
		horizontalRes,
		verticalRes,
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
	usingSecondFrame = false;
	frameCounter = 0;
}

void Renderer::beginFrame(void) {
    auto newChain = getCurrentChain();

    // determine where new framebuffer to draw to is in vram
    int bufferX = usingSecondFrame ? SCREEN_WIDTH : 0;
    int bufferY = 0;

    // clear and prepare new chain
    clearOrderingTable(newChain->orderingTable, ORDERING_TABLE_SIZE);
    newChain->nextPacket = newChain->data;

    // add gpu commands to clear buffer and set drawing origin to new chain
    // z is set to (ORDERING_TABLE_SIZE - 1) so they're executed before anything else
    uint32_t bgColor = gp0_rgb(64, 64, 64);

    auto ptr = allocatePacket(ORDERING_TABLE_SIZE - 1, 7);
    ptr[0]   = gp0_texpage(0, true, false);
    ptr[1]   = gp0_xy(bufferX, bufferY);
    ptr[2]   = gp0_fbOffset2(bufferX + SCREEN_WIDTH -  1, bufferY + SCREEN_HEIGHT - 2);
    ptr[3]   = gp0_fbOrigin(bufferX, bufferY);
    ptr[4]   = bgColor | gp0_vramFill();
    ptr[5]   = gp0_xy(bufferX, bufferY);
    ptr[6]   = gp0_xy(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void Renderer::endFrame(void) {
    auto oldChain = getCurrentChain();

    // switch active chain
    usingSecondFrame = !usingSecondFrame;

    // determine where new framebuffer to display is in vram
    int bufferX = usingSecondFrame ? SCREEN_WIDTH : 0;
    int bufferY = 0;

    // display new framebuffer after vsync
    waitForGP0Ready();
    waitForVSync();
    GPU_GP1 = gp1_fbOffset(bufferX, bufferY);

    // terminate and start drawing current chain
    *(oldChain->nextPacket) = gp0_endTag(0);
    sendLinkedList(&(oldChain->orderingTable)[ORDERING_TABLE_SIZE - 1]);
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
    auto *ptr = allocatePacket(z, 10);
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

void Renderer::drawModel(const ModelFile *model, int tx, int ty, int tz, int rotX, int rotY, int rotZ) {
	gte_setControlReg(GTE_TRX,  tx);
	gte_setControlReg(GTE_TRY,  ty);
	gte_setControlReg(GTE_TRZ,  tz);
	gte_setRotationMatrix(
		ONE,   0,   0,
		  0, ONE,   0,
		  0,   0, ONE
	);

	GTE::rotateCurrentMatrix(rotX, rotY, rotZ);

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
		gte_command(GTE_CMD_NCLIP);

		if (gte_getDataReg(GTE_MAC0) <= 0)
			continue;
		
		// Save the first transformed vertex (the GTE only keeps the X/Y
		// coordinates of the last 3 vertices processed and Z coordinates of
		// the last 4 vertices processed) and apply projection to the last
		// vertex.
		uint32_t xy0 = gte_getDataReg(GTE_SXY0);
		if (!istriangle) {
			gte_loadV0(&model->vertices[face->indices[3]]);
			gte_command(GTE_CMD_RTPS | GTE_SF);
		}
		// Calculate the average Z coordinate of all vertices and use it to
		// determine the ordering table bucket index for this face.
		gte_command(GTE_CMD_AVSZ4 | GTE_SF);
		int zIndex = gte_getDataReg(GTE_OTZ);

		if ((zIndex < 0) || (zIndex >= ORDERING_TABLE_SIZE))
			continue;

		// Create a new quad and give its vertices the X/Y coordinates
		// calculated by the GTE.
		if (istriangle) {//face is a triangle
			if (face->texid < 0) {// not textured
				auto ptr = allocatePacket(zIndex,6);
				ptr[0] = face->color | gp0_shadedTriangle(true, false, false);
				ptr[1] = xy0;
				ptr[2] = face->color;
				gte_storeDataReg(GTE_SXY1, 3 * 4, ptr);
				ptr[4] = face->color;
				gte_storeDataReg(GTE_SXY2, 5 * 4, ptr);
			}
			else { //textured
				auto clut = model->textures[face->texid].clut;
				auto page = model->textures[face->texid].page;
				auto *ptr = allocatePacket(zIndex, 8);
				ptr[0]        = gp0_texpage(page, false, false); // set texture page and CLUT
				ptr[1]        = face->color | gp0_shadedTriangle(false, true, false);
				ptr[2]        = xy0;
				ptr[3]        = gp0_uv(face->u[0], face->v[0], clut);
				gte_storeDataReg(GTE_SXY1, 4 * 4, ptr);
				ptr[5]        = gp0_uv(face->u[1], face->v[1], page);
				gte_storeDataReg(GTE_SXY2, 6 * 4, ptr);
				ptr[7]        = gp0_uv(face->u[2], face->v[2], 0);
			}
		}
		else { //face is a quad
			
			if (face->texid < 0) { //not textured 
				auto ptr    = allocatePacket(zIndex, 5);
				ptr[0] = face->color | gp0_shadedQuad(false, false, false);
				ptr[1] = xy0;
				gte_storeDataReg(GTE_SXY0, 2 * 4, ptr);
				gte_storeDataReg(GTE_SXY1, 3 * 4, ptr);
				gte_storeDataReg(GTE_SXY2, 4 * 4, ptr);
			}
			else { //textured 
				auto clut = model->textures[face->texid].clut;
				auto page = model->textures[face->texid].page;
			    auto *ptr = allocatePacket(zIndex, 10);
				ptr[0]    = gp0_texpage(page, false, false); // set texture page and CLUT
				ptr[1]    = face->color | gp0_shadedQuad(false, true, false);
				ptr[2]    = xy0;
				ptr[3]    = gp0_uv(face->u[0], face->v[0], clut);
				gte_storeDataReg(GTE_SXY0, 4 * 4, ptr);
				ptr[5]    = gp0_uv(face->u[1], face->v[1], page);
				gte_storeDataReg(GTE_SXY1, 6 * 4, ptr);
				ptr[7]    = gp0_uv(face->u[2], face->v[2], 0);
				gte_storeDataReg(GTE_SXY2, 8 * 4, ptr);
				ptr[9]    = gp0_uv(face->u[3], face->v[3], 0);
			}

			/*
				//(A, B, C) and (B, C, D) respectively;

				//tri1
				uint32_t *ptr = allocatePacket(zIndex,6);
				ptr[0] = gp0_shadedTriangle(true, false, false) | gp0_rgb(255, 0, 0);
				ptr[1] = xy0;
				ptr[2] = gp0_rgb(0, 255, 0);
				gte_storeDataReg(GTE_SXY0, 3 * 4, ptr);
				ptr[4] = gp0_rgb(0, 0, 255);
				gte_storeDataReg(GTE_SXY1, 5 * 4, ptr);
				//tri2
				ptr = allocatePacket(zIndex,6);
				ptr[0] = gp0_shadedTriangle(true, false, false) | gp0_rgb(255, 0, 0);
				gte_storeDataReg(GTE_SXY0, 1 * 4, ptr);
				ptr[2] = gp0_rgb(0, 255, 0);
				gte_storeDataReg(GTE_SXY1, 3 * 4, ptr);
				ptr[4] = gp0_rgb(0, 0, 255);
				gte_storeDataReg(GTE_SXY2, 5 * 4, ptr);*/
		}
	}
}

uint32_t *Renderer::allocatePacket(int zIndex, int numCommands) {
    auto chain = getCurrentChain();
    auto ptr   = chain->nextPacket;

    // check z index is valid
    assert((zIndex >= 0) && (zIndex < ORDERING_TABLE_SIZE));

    // link new packet into ordering table at specified z index
    *ptr = gp0_tag(numCommands, (void *) chain->orderingTable[zIndex]);
    chain->orderingTable[zIndex] = gp0_tag(0, ptr);

    // bump up allocator and check we haven't run out of space
    chain->nextPacket += numCommands + 1;
    assert(chain->nextPacket < &(chain->data)[CHAIN_BUFFER_SIZE]);

    return &ptr[1];
}

void Renderer::printString(XY<int32_t> pos, int zIndex, const char *str) {
	assert(fontData);
	int currentX = pos.x, currentY = pos.y;

	uint32_t *ptr;

	for (; *str; str++) {
		char ch = *str;

		// Check if the character is "special"
		int tabwidth = fontData->header->tabwidth;
		switch (ch) {
			case '\t':
				currentX += tabwidth - 1;
				currentX -= currentX % tabwidth;
				continue;

			case '\n':
				currentX  = pos.x;
				currentY += fontData->header->lineheight;
				continue;

			case ' ':
				currentX += fontData->header->spacewidth;
				continue;

			case '\x80' ... '\xff':
				ch = '\x7f';
				break;
		}

		const RECT<uint8_t> &rect = fontData->rects[ch - fontData->header->firstchar];
		
		// Enable blending to make sure any semitransparent pixels in the font get rendered correctly.
		ptr    = allocatePacket(zIndex, 4);
		ptr[0] = gp0_rectangle(true, true, true);
		ptr[1] = gp0_xy(currentX, currentY);
		ptr[2] = gp0_uv(fontTex.u + rect.x, fontTex.v + rect.y, fontTex.clut);
		ptr[3] = gp0_xy(rect.w, rect.h);

		currentX += rect.w;
	}
	
	ptr    = allocatePacket(zIndex, 1);
	ptr[0] = gp0_texpage(fontTex.page, false, false);
}


//indexed
void uploadTexture(TextureInfo &info, const void *image) {
    const TexHeader* header = reinterpret_cast<const TexHeader*>(image);
	assert(header->isValid());
	info = header->texinfo;

	assert((info.w <= 256) && (info.h <= 256));

	int numColors = (info.bpp == 8) ? 256 : 16;
	int widthDivider = (info.bpp == 8) ? 2 : 4;

	sendVRAMData(header->texdata(), {header->vrampos[0], header->vrampos[1], info.w / widthDivider, info.h});
	waitForDMADone();
	sendVRAMData(header->clut(), {header->clutpos[0], header->clutpos[1], numColors, 1});
	waitForDMADone();
}

const ModelFile* loadModel(const uint8_t* data) {
    auto model = new ModelFile(); 
    model->header = reinterpret_cast<const ModelFileHeader*>(data);
	assert(model->header->isValid());

    model->vertices = model->header->vertices();
    model->faces = model->header->faces();

	//textures
	auto texptr = model->header->textures();

	for (int i = 0; i < static_cast<int>(model->header->numtex); i++) {
		const TexHeader* texheader = reinterpret_cast<const TexHeader*>(texptr);
		uploadTexture(model->textures[i], texptr);
		
		size_t clut_bytes = texheader->clutsize;
		size_t tex_bytes = texheader->texsize;

		texptr += sizeof(TexHeader) + clut_bytes + tex_bytes;
	}

    return model;
}

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
	size_t chunkSize, numChunks;

	if (length < DMA_MAX_CHUNK_SIZE) {
		chunkSize = length;
		numChunks = 1;
	} else {
		chunkSize = DMA_MAX_CHUNK_SIZE;
		numChunks = length / DMA_MAX_CHUNK_SIZE;

		assert(!(length % DMA_MAX_CHUNK_SIZE));
	}

	waitForGP0Ready();
	GPU_GP0 = gp0_vramWrite();
	GPU_GP0 = gp0_xy(rect.x, rect.y);
	GPU_GP0 = gp0_xy(rect.w, rect.h);

	DMA_MADR(DMA_GPU) = (uint32_t) data;
	DMA_BCR (DMA_GPU) = chunkSize | (numChunks << 16);
	DMA_CHCR(DMA_GPU) = 0
		| DMA_CHCR_WRITE
		| DMA_CHCR_MODE_SLICE
		| DMA_CHCR_ENABLE;
}

static void clearOrderingTable(uint32_t *table, int numEntries) {
	DMA_MADR(DMA_OTC) = (uint32_t) &table[numEntries - 1];
	DMA_BCR (DMA_OTC) = numEntries;
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