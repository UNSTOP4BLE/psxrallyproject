#include "app.hpp"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "ps1/registers.h"

#include "scenes/test.hpp"

extern const uint8_t g_myfontmap[];
extern const uint8_t g_myfonttex[];

App g_app;

int main(int argc, const char **argv) {
	initSerialIO(115200);

	GP1VideoMode mode;
	if ((GPU_GP1 & GP1_STAT_FB_MODE_BITMASK) == GP1_STAT_FB_MODE_PAL) {
		puts("Using PAL mode");
		mode = GP1_MODE_PAL;
	} else {
		puts("Using NTSC mode");
		mode = GP1_MODE_NTSC;
	}
	g_app.renderer.init(mode);
	
	GTE::setupGTE();
	GFX::uploadTexture(g_app.renderer.fonttex, g_myfonttex);
	g_app.renderer.fontmap = GFX::loadFontMap(g_myfontmap);

	SCENE::set(new TestSCN());

	while(1) {
		g_app.renderer.beginFrame();
		
		assert(g_app.curscene);
     
        g_app.curscene->update();  
        g_app.curscene->draw();  

		g_app.renderer.printStringf({5, 5}, 0, "Heap usage: %zu/%zu bytes", getHeapUsage(), 0x80200000);
		g_app.renderer.endFrame();
	}
	return 0;
}
