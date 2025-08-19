/*
 * ps1-bare-metal - (C) 2023-2025 spicyjpeg
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "gpu.h"
#include "ps1/gpucmd.h"
#include "ps1/registers.h"

namespace GFX {
static void waitForGP0Ready(void);
static void waitForDMADone(void);
static void waitForVSync(void);
static void sendLinkedList(const void *data);
static void sendVRAMData(const void *data, Rect rect);
static void clearOrderingTable(uint32_t *table, int numEntries);
static uint32_t *allocatePacket(DMAChain *chain, int zIndex, int numCommands);

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
}

void Renderer::clear(void) {
	int bufferX = usingSecondFrame ? SCREEN_WIDTH : 0;
	int bufferY = 0;

	chain  = &dmaChains[usingSecondFrame];
	usingSecondFrame = !usingSecondFrame;
	
	uint32_t *ptr = nullptr;

	GPU_GP1 = gp1_fbOffset(bufferX, bufferY);

	clearOrderingTable(chain->orderingTable, ORDERING_TABLE_SIZE);
	chain->nextPacket = chain->data;

	ptr    = allocatePacket(chain, 0, 4);
	ptr[0] = gp0_texpage(0, true, false);
	ptr[1] = gp0_xy(bufferX, bufferY);
	ptr[2] = gp0_fbOffset2(
		bufferX + SCREEN_WIDTH -  1,
		bufferY + SCREEN_HEIGHT - 2
	);
	ptr[3] = gp0_fbOrigin(bufferX, bufferY);

	ptr    = allocatePacket(chain, 0, 3);
	ptr[0] = gp0_rgb(0, 255, 0) | gp0_vramFill();
	ptr[1] = gp0_xy(bufferX, bufferY);
	ptr[2] = gp0_xy(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void Renderer::flip(void) {
	*(chain->nextPacket) = gp0_endTag(0);
	waitForGP0Ready();
	waitForVSync();
	sendLinkedList(&(chain->orderingTable)[ORDERING_TABLE_SIZE - 1]);
}

void Renderer::drawRect(Rect rect, int r, int g, int b) {
	uint32_t *ptr = allocatePacket(chain, 0, 3);
	ptr[0]        = gp0_rgb(r, g, b) | gp0_rectangle(false, false, false); // solid, untextured
	ptr[1]        = gp0_xy(rect.x, rect.y);          // top-left corner
	ptr[2]        = gp0_xy(rect.w, rect.h);       // width and height
}

void Renderer::drawTexRect(TextureInfo &tex, Pos pos) {
	uint32_t *ptr = allocatePacket(chain, 0, 5);
	ptr[0]        = gp0_texpage(tex.page, false, false);
	ptr[1]        = gp0_rectangle(true, true, false);
	ptr[2]        = gp0_xy(pos.x, pos.y);
	ptr[3]        = gp0_uv(tex.u, tex.v, 0);
	ptr[4]        = gp0_xy(tex.width, tex.height);
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

static uint32_t *allocatePacket(DMAChain *chain, int zIndex, int numCommands) {
	uint32_t *ptr      = chain->nextPacket;
	chain->nextPacket += numCommands + 1;

	assert((zIndex >= 0) && (zIndex < ORDERING_TABLE_SIZE));

	*ptr = gp0_tag(numCommands, (void *) chain->orderingTable[zIndex]);
	chain->orderingTable[zIndex] = gp0_tag(0, ptr);

	assert(chain->nextPacket < &(chain->data)[CHAIN_BUFFER_SIZE]);

	return &ptr[1];
}
}