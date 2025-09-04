#include "app.h"
#include <stdio.h>
#include "ps1/registers.h"

extern const uint8_t g_mytex[];
extern const uint8_t g_mymodel[];
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
    //auto model = GFX::loadModel(mymodel);
	int _x = 0;
	while(1) {
		g_app.renderer.beginFrame();
		
//		_x += 10;
//		g_app.renderer.drawTexQuad(g_app.renderer.fonttex, {0, 0, 320, 240}, 0, 0x808080);
    	//app.renderer.drawModel(model,
        //	  0, 0, 0,                  // translation
          //    0, x, 90);
		g_app.renderer.printString({50, 50}, "hello world!", 0);
		g_app.renderer.endFrame();
	}
	return 0;
}
