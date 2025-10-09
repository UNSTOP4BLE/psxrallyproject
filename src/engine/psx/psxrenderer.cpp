#include "../renderer.hpp"
#include <assert.h>
#include <stdio.h> //puts
#include <ps1/registers.h>
#include <ps1/gpucmd.h>
#include <ps1/system.h>

namespace ENGINE::PSX {
	//helpers
	static void waitForGP0Ready(void);
	static void waitForDMADone(void);
	static void clearOT(uint32_t *table, int numentries);
	static void sendLinkedList(const void *data);

		
	PSXRenderer::PSXRenderer(void) {
		GP1VideoMode mode;
		if ((GPU_GP1 & GP1_STAT_FB_MODE_BITMASK) == GP1_STAT_FB_MODE_PAL) {
			puts("Using PAL mode");
			mode = GP1_MODE_PAL;
			refreshrate = 50;
		} else {
			puts("Using NTSC mode");
			mode = GP1_MODE_NTSC;
			refreshrate = 60;
		}

		// Set the origin of the displayed framebuffer. These "magic" values,
		// derived from the GPU's internal clocks, will center the picture on most
		// displays and upscalers.
		int x = 0x760;
		int y = (mode == GP1_MODE_PAL) ? 0xa3 : 0x88;

		// horizontal (256, 320, 368, 512, 640) and vertical (240-256, 480-512) resolutions to pick
		GP1HorizontalRes hres;
		scrw = ENGINE::COMMON::SCREEN_WIDTH;
		scrh = ENGINE::COMMON::SCREEN_HEIGHT;
		
		switch (scrw) {
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
		GP1VerticalRes vres = (scrh > 256) ? GP1_VRES_512 : GP1_VRES_256;

		int offx = (scrw  * gp1_clockMultiplierH(hres)) / 2;
		int offy = (scrh / gp1_clockDividerV(vres))    / 2;

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
		framecounter = vsynccounter = fps = 0;
		setClearCol(64,64,64);
		clearcol = 0x888888;
	}

	void PSXRenderer::beginFrame(void) {
		auto newchain = getCurrentChain();

		// determine where new framebuffer to draw to is in vram
		int bufx = 0;
		int bufy = usingsecondframe ? scrh : 0;

		// clear and prepare new chain
		clearOT(newchain->orderingtable, ENGINE::COMMON::ORDERING_TABLE_SIZE);
		newchain->nextpacket = newchain->data;

		// add gpu commands to clear buffer and set drawing origin to new chain
		// z is set to (ORDERING_TABLE_SIZE - 1) so they're executed before anything else
		auto ptr = allocatePacket(ENGINE::COMMON::ORDERING_TABLE_SIZE - 1, 7);
		ptr[0]   = gp0_texpage(0, true, false);
		ptr[1]   = gp0_fbOffset1(bufx, bufy);
		ptr[2]   = gp0_fbOffset2(bufx + scrw -  1, bufy + scrh - 2);
		ptr[3]   = gp0_fbOrigin(bufx, bufy);
		ptr[4]   = clearcol | gp0_vramFill();
		ptr[5]   = gp0_xy(bufx, bufy);
		ptr[6]   = gp0_xy(scrw, scrh);
	}
		
	void PSXRenderer::endFrame(void) {
		auto oldchain = getCurrentChain();

		// switch active chain
		usingsecondframe = !usingsecondframe;

		// determine where new framebuffer to display is in vram
		int bufx = 0;
		int bufy = usingsecondframe ? scrh : 0;

		// display new framebuffer after vsync
		waitForGP0Ready();
		waitForVSync();
		GPU_GP1 = gp1_fbOffset(bufx, bufy);

		// terminate and start drawing current chain
		*(oldchain->nextpacket) = gp0_endTag(0);
		sendLinkedList(&(oldchain->orderingtable)[ENGINE::COMMON::ORDERING_TABLE_SIZE - 1]);
	}

	void PSXRenderer::drawRect(ENGINE::COMMON::RECT32 rect, int z, uint32_t col) {
		auto ptr      = allocatePacket(z, 3);
		ptr[0]        = col | gp0_rectangle(false, false, false); 
		ptr[1]        = gp0_xy(rect.x, rect.y);       
		ptr[2]        = gp0_xy(rect.w, rect.h);  
	}

	//void PSXRenderer::drawTexRect(const TextureInfo &tex, ENGINE::COMMON::XY<int32_t> pos, int z, int col) {}
	//void PSXRenderer::drawTexQuad(const TextureInfo &tex, ENGINE::COMMON::RECT<int32_t> pos, int z, uint32_t col) {}
	//void PSXRenderer::drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot) {}
	void PSXRenderer::printString(ENGINE::COMMON::XY32 pos, int z, const char *str) {}
	void PSXRenderer::printStringf(ENGINE::COMMON::XY32 pos, int z, const char *fmt, ...) {}

	void PSXRenderer::handleVSyncInterrupt(void) {
		__atomic_signal_fence(__ATOMIC_ACQUIRE);
		
		vsynccounter++;
		if (vsynccounter >= refreshrate) {
			fps = framecounter;
			vsynccounter -= refreshrate;
			framecounter = 0;
		}

		__atomic_signal_fence(__ATOMIC_RELEASE);
	}


	//helpers
	static void waitForGP0Ready(void) {
		while (!(GPU_GP1 & GP1_STAT_CMD_READY))
			__asm__ volatile("");
	}

	static void waitForDMADone(void) {
		while (DMA_CHCR(DMA_GPU) & DMA_CHCR_ENABLE)
			__asm__ volatile("");
	}

	void PSXRenderer::waitForVSync(void) {
		uint32_t lastcounter = vsynccounter;

		framecounter++;
		__atomic_signal_fence(__ATOMIC_RELEASE);

		// wait for up to 25ms (vsync is every 16.6ms at 60hz or every 20ms at 50hz)
		for (int i = 2500; i > 0; i--) {
			__atomic_signal_fence(__ATOMIC_ACQUIRE);

			if (vsynccounter != lastcounter)
				return;

			delayMicroseconds(10);
		}

		printf("timeout while waiting for vsync! something has gone horribly wrong\n");
	}

	uint32_t *PSXRenderer::allocatePacket(int z, int numcommands) {
		auto chain = getCurrentChain();
		auto ptr   = chain->nextpacket;

		// check z index is valid
		assert((z >= 0) && (z < ENGINE::COMMON::ORDERING_TABLE_SIZE));

		// link new packet into ordering table at specified z index
		*ptr = gp0_tag(numcommands, reinterpret_cast<void *>(chain->orderingtable[z]));
		chain->orderingtable[z] = gp0_tag(0, ptr);

		// bump up allocator and check we haven't run out of space
		chain->nextpacket += numcommands + 1;
		assert(chain->nextpacket < &(chain->data)[ENGINE::COMMON::CHAIN_BUFFER_SIZE]);

		return &ptr[1];
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

	static void sendLinkedList(const void *data) {
		waitForDMADone();
		assert(!((uint32_t) data % 4));

		DMA_MADR(DMA_GPU) = (uint32_t) data;
		DMA_CHCR(DMA_GPU) = 0
			| DMA_CHCR_WRITE
			| DMA_CHCR_MODE_LIST
			| DMA_CHCR_ENABLE;
	}

}