#include "gpu.h"
#include <assert.h>
#include <stdbool.h>
#include "ps1/registers.h"
#include <stdlib.h>
#include <stdio.h>

namespace GFX {
static void waitForGP0Ready(void);
static void waitForDMADone(void);
static void waitForVSync(void);
static void sendLinkedList(const void *data);
static void sendVRAMData(const void *data, Rect rect);
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

void Renderer::drawRect(Rect rect, int z, uint32_t col) {
	auto ptr = allocatePacket(0, 3);
	ptr[0]        = col | gp0_rectangle(false, false, false); 
	ptr[1]        = gp0_xy(rect.x, rect.y);       
	ptr[2]        = gp0_xy(rect.w, rect.h);     
}

void Renderer::drawTexRect(const TextureInfo &tex, XY pos, int z, int col) {
	auto ptr = allocatePacket(0, 5);
	ptr[0]        = gp0_texpage(tex.page, false, false);
	ptr[1]        = col | gp0_rectangle(true, true, false);
	ptr[2]        = gp0_xy(pos.x, pos.y);
	ptr[3]        = gp0_uv(tex.u, tex.v, 0);
	ptr[4]        = gp0_xy(tex.width, tex.height);
}

void Renderer::drawTri(XY v0, XY v1, XY v2, int z, uint32_t col) {
    auto ptr = allocatePacket(z, 4);
    ptr[0]        = col | gp0_shadedTriangle(false, false, false);
    ptr[1]        = gp0_xy(v0.x, v0.y);
    ptr[2]        = gp0_xy(v1.x, v1.y);
    ptr[3]        = gp0_xy(v2.x, v2.y);
}

void Renderer::drawTexTri(const TextureInfo &tex, XY v0, XY v1, XY v2, XY uv0, XY uv1, XY uv2, int z, uint32_t col) {
    auto *ptr = allocatePacket(z, 8);
	ptr[0]        = gp0_texpage(tex.page, false, false); // set texture page and CLUT
    ptr[1]        = col | gp0_shadedTriangle(false, true, false);
    ptr[2]        = gp0_xy(v0.x, v0.y);
    ptr[3]        = gp0_uv(uv0.x, uv0.y, 0);
    ptr[4]        = gp0_xy(v1.x, v1.y);
    ptr[5]        = gp0_uv(uv1.x, uv1.y, tex.page);
    ptr[6]        = gp0_xy(v2.x, v2.y);
    ptr[7]        = gp0_uv(uv2.x, uv2.y, 0);
}

void Renderer::drawQuad(XY v0, XY v1, XY v2, XY v3, int z, uint32_t col) {
	auto ptr    = allocatePacket(z, 5);
	ptr[0] = col | gp0_shadedQuad(false, false, false);
	ptr[1] = gp0_xy(v0.x, v0.y);
	ptr[2] = gp0_xy(v1.x, v1.y);
	ptr[3] = gp0_xy(v2.x, v2.y);
	ptr[4] = gp0_xy(v3.x, v3.y);
}

void Renderer::drawTexQuad(const TextureInfo &tex, XY v0, XY v1, XY v2, XY v3, XY uv0, XY uv1, XY uv2, XY uv3, int z, uint32_t col) {
    auto *ptr = allocatePacket(z, 10);
	ptr[0]    = gp0_texpage(tex.page, false, false); // set texture page and CLUT
    ptr[1]    = col | gp0_shadedQuad(false, true, false);
    ptr[2]    = gp0_xy(v0.x, v0.y);
    ptr[3]    = gp0_uv(uv0.x, uv0.y, 0);
    ptr[4]    = gp0_xy(v1.x, v1.y);
    ptr[5]    = gp0_uv(uv1.x, uv1.y, tex.page);
    ptr[6]    = gp0_xy(v2.x, v2.y);
    ptr[7]    = gp0_uv(uv2.x, uv2.y, 0);
    ptr[8]    = gp0_xy(v3.x, v3.y);
    ptr[9]    = gp0_uv(uv3.x, uv3.y, 0);
}

//correct
static const Face cubeFaces[6] = {
    { .indices = {2, 6, 0, 4}, .color = 0x0000ff },
    { .indices = {6, 2, 7, 3}, .color = 0x00ff00 },
    { .indices = {6, 7, 4, 5}, .color = 0x00ffff },
    { .indices = {7, 3, 5, 1}, .color = 0xff0000 },
    { .indices = {3, 2, 1, 0}, .color = 0xff00ff },
    { .indices = {5, 1, 4, 0}, .color = 0xffff00 }
};

//made by converter
static const Face cubeFacesconv[6] = {
    { .indices = { 0, 4, 6, 2 }, .color = 0x0000ff },
    { .indices = { 3, 2, 6, 7 }, .color = 0x00ff00 },
    { .indices = { 7, 6, 4, 5 }, .color = 0x00ffff },
    { .indices = { 5, 1, 3, 7 }, .color = 0xff0000 },
    { .indices = { 1, 0, 2, 3 }, .color = 0xff00ff },
    { .indices = { 5, 4, 0, 1 }, .color = 0xffff00 }
};

void Renderer::drawModel(const ModelFile *model, int tx, int ty, int tz, int rotX, int rotY, int rotZ, const TextureInfo &tex) {
	gte_setControlReg(GTE_TRX,  tx);
	gte_setControlReg(GTE_TRY,  ty);
	gte_setControlReg(GTE_TRZ,  tz);
	gte_setRotationMatrix(
		ONE,   0,   0,
		  0, ONE,   0,
		  0,   0, ONE
	);

	GTE::rotateCurrentMatrix(rotX, rotY, rotZ);

	for (int i = 0; i < static_cast<int>(model->header.numfaces); i++) {
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
			auto ptr = allocatePacket(zIndex,6);
			ptr[0] = gp0_shadedTriangle(true, false, false) | gp0_rgb(255, 0, 0);
			ptr[1] = xy0;
			ptr[2] = gp0_rgb(0, 255, 0);
			gte_storeDataReg(GTE_SXY1, 3 * 4, ptr);
			ptr[4] = gp0_rgb(0, 0, 255);
			gte_storeDataReg(GTE_SXY2, 5 * 4, ptr);
			
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
			    auto *ptr = allocatePacket(zIndex, 10);
				ptr[0]    = gp0_texpage(tex.page, false, false); // set texture page and CLUT
				ptr[1]    = face->color | gp0_shadedQuad(false, true, false);
				ptr[2]    = xy0;
				ptr[3]    = gp0_uv(face->u[0], face->v[0], 0);
				gte_storeDataReg(GTE_SXY0, 4 * 4, ptr);
				ptr[5]    = gp0_uv(face->u[1], face->v[1], tex.page);
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

void uploadTexture(TextureInfo &info, const void *data, Rect pos) {
	assert((pos.w <= 256) && (pos.h <= 256));

	sendVRAMData(data, pos);
	waitForDMADone();

	info.page   = gp0_page(
		pos.x /  64,
		pos.y / 256,
		GP0_BLEND_SEMITRANS,
		GP0_COLOR_16BPP
	);
	info.clut   = 0;
	info.u      = (uint8_t)  (pos.x %  64);
	info.v      = (uint8_t)  (pos.y % 256);
	info.width  = (uint16_t) pos.w;
	info.height = (uint16_t) pos.h;
}

void uploadIndexedTexture(TextureInfo &info, const void *image, const void *palette, XY palleteXY, Rect imgrect, GP0ColorDepth colorDepth) {
	assert((imgrect.w <= 256) && (imgrect.h <= 256));

	int numColors    = (colorDepth == GP0_COLOR_8BPP) ? 256 : 16;
	int widthDivider = (colorDepth == GP0_COLOR_8BPP) ?   2 :  4;

	assert(!(palleteXY.x % 16) && ((palleteXY.x + numColors) <= 1024));

	sendVRAMData(image, {imgrect.x, imgrect.y, imgrect.w / widthDivider, imgrect.h});
	waitForDMADone();
	sendVRAMData(palette, {palleteXY.x, palleteXY.y, numColors, 1});
	waitForDMADone();

	info.page   = gp0_page(imgrect.x / 64, imgrect.y / 256, GP0_BLEND_SEMITRANS, colorDepth);
	info.clut   = gp0_clut(palleteXY.x / 16, palleteXY.y);
	info.u      = (uint8_t)  ((imgrect.x %  64) * widthDivider);
	info.v      = (uint8_t)   (imgrect.y % 256);
	info.width  = (uint16_t) imgrect.w;
	info.height = (uint16_t) imgrect.h;
}
const ModelFile* loadModel(const uint8_t* data) {
    // reinterpret header
    const ModelFileHeader* header = reinterpret_cast<const ModelFileHeader*>(data);
 //   if (!header->isValid()) {
   //     printf("Invalid model!\n");
    //    return nullptr;
    //}

    // set up ModelFile
    static ModelFile model; // or malloc if you prefer
    model.header = *header; 

    // vertices are right after the header
    model.vertices = reinterpret_cast<const GTEVector16*>(header->vertices());

    // faces are right after the vertices
    model.faces = reinterpret_cast<const Face*>(header->faces());

    printf("Model Header:\n");
    printf("  Magic: %u\n", model.header.magic);
    printf("  Num Vertices: %u\n", model.header.numvertices);
    printf("  Num Faces: %u\n", model.header.numfaces);

    printf("\nVertices:\n");
    for (uint32_t i = 0; i < model.header.numvertices; i++) {
        printf("  [%u] x=%d y=%d z=%d\n", i, model.vertices[i].x, model.vertices[i].y, model.vertices[i].z);
    }

    printf("\nFaces:\n");
    for (uint32_t i = 0; i < model.header.numfaces; i++) {
        const Face &f = model.faces[i];
        printf("  [%u] indices: %u, %u, %u, %u  color: 0x%08X\n",
            i, f.indices[0], f.indices[1], f.indices[2], f.indices[3], f.color);
    }

    return &model;
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

static void sendVRAMData(const void *data, Rect rect) {
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