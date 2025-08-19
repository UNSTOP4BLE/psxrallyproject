#include "gpu.h"
#include <assert.h>
#include <stdbool.h>
#include "ps1/registers.h"

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

void Renderer::drawRect(Rect rect, int r, int g, int b) {
	uint32_t *ptr = allocatePacket(0, 3);
	ptr[0]        = gp0_rgb(r, g, b) | gp0_rectangle(false, false, false); // solid, untextured
	ptr[1]        = gp0_xy(rect.x, rect.y);          // top-left corner
	ptr[2]        = gp0_xy(rect.w, rect.h);       // width and height
}

void Renderer::drawTexTri(TextureInfo &tex, Pos v0, Pos v1, Pos v2, Pos uv0, Pos uv1, Pos uv2) {
   	// Draw first triangle
    uint32_t *ptr = allocatePacket(0, 8);
	ptr[0]        = gp0_texpage(tex.page, false, false); // set texture page and CLUT
    ptr[1]        = gp0_rgb(128, 128, 128) | gp0_shadedTriangle(false, true, false);
    ptr[2]        = gp0_xy(v0.x, v0.y);
    ptr[3]        = gp0_uv(uv0.x, uv0.y, 0);
    ptr[4]        = gp0_xy(v1.x, v1.y);
    ptr[5]        = gp0_uv(uv1.x, uv1.y, tex.page);
    ptr[6]        = gp0_xy(v2.x, v2.y);
    ptr[7]        = gp0_uv(uv2.x, uv2.y, 0);
}

void Renderer::drawTexQuad(TextureInfo &tex, Pos v0, Pos v1, Pos v2, Pos v3, Pos uv0, Pos uv1, Pos uv2, Pos uv3) {
    drawTexTri(tex, v0, v1, v2, uv0, uv1, uv2);
    drawTexTri(tex, v3, v0, v2, uv3, uv0, uv2);
}

void Renderer::drawTexRect(TextureInfo &tex, Pos pos) {
	uint32_t *ptr = allocatePacket(0, 5);
	ptr[0]        = gp0_texpage(tex.page, false, false);
	ptr[1]        = gp0_rectangle(true, true, false);
	ptr[2]        = gp0_xy(pos.x, pos.y);
	ptr[3]        = gp0_uv(tex.u, tex.v, 0);
	ptr[4]        = gp0_xy(tex.width, tex.height);
}

void Renderer::drawModel(const Model *model, int tx, int ty, int tz, int rotX, int rotY, int rotZ) {
    // Setup GTE transform
    gte_setControlReg(GTE_TRX, tx);
    gte_setControlReg(GTE_TRY, ty);
    gte_setControlReg(GTE_TRZ, tz);

    gte_setRotationMatrix(
        ONE,   0,   0,
          0, ONE,   0,
          0,   0, ONE
    );

    GTE::rotateCurrentMatrix(rotX, rotY, rotZ);

    // Walk faces
    for (int i = 0; i < model->numFaces; i++) {
        const GTE::Face *face = &model->faces[i];

        // Transform first 3 vertices
        gte_loadV0(&model->vertices[face->vertices[0]]);
        gte_loadV1(&model->vertices[face->vertices[1]]);
        gte_loadV2(&model->vertices[face->vertices[2]]);
        gte_command(GTE_CMD_RTPT | GTE_SF);

        // Backface cull
        gte_command(GTE_CMD_NCLIP);
        if (gte_getDataReg(GTE_MAC0) <= 0)
            continue;

        uint32_t xy0 = gte_getDataReg(GTE_SXY0);

        // Transform 4th vertex
        gte_loadV0(&model->vertices[face->vertices[3]]);
        gte_command(GTE_CMD_RTPS | GTE_SF);

        // Depth average
        gte_command(GTE_CMD_AVSZ4 | GTE_SF);
        int zIndex = gte_getDataReg(GTE_OTZ);
        if ((zIndex < 0) || (zIndex >= ORDERING_TABLE_SIZE))
            continue;

        // Emit quad into renderer’s chain
        uint32_t *ptr = allocatePacket(zIndex, 5);
        ptr[0] = face->color | gp0_shadedQuad(false, false, false);
        ptr[1] = xy0;
        gte_storeDataReg(GTE_SXY0, 2 * 4, ptr);
        gte_storeDataReg(GTE_SXY1, 3 * 4, ptr);
        gte_storeDataReg(GTE_SXY2, 4 * 4, ptr);
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

void uploadIndexedTexture(TextureInfo &info, const void *image, const void *palette, Pos palleteXY, Rect imgrect, GP0ColorDepth colorDepth) {
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