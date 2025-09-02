#include "app.h"
#include <stdio.h>
#include "ps1/registers.h"

extern const uint8_t mytex[];
extern const uint8_t mymodel[];
extern const uint8_t myfont[];
extern const uint8_t myfonttex[];

App app;
int main(int argc, const char **argv) {
	initSerialIO(115200);

	if ((GPU_GP1 & GP1_STAT_FB_MODE_BITMASK) == GP1_STAT_FB_MODE_PAL) {
		puts("Using PAL mode");
		app.renderer.init(GP1_MODE_PAL, SCREEN_WIDTH, SCREEN_HEIGHT);
	} else {
		puts("Using NTSC mode");
		app.renderer.init(GP1_MODE_NTSC, SCREEN_WIDTH, SCREEN_HEIGHT);
	}

	GTE::setupGTE(SCREEN_WIDTH, SCREEN_HEIGHT);
	GFX::uploadTexture(app.renderer.fontTex, myfonttex);
	app.renderer.fontData = GFX::loadFontMap(myfont);
    //auto model = GFX::loadModel(mymodel);
	int x = 0;
	while(1) {
		app.renderer.beginFrame();
		
		x += 10;
		//app.renderer.drawTexQuad(app.renderer.fontTex, {0, 0, 320, 240}, 0, 0x808080);
    	//app.renderer.drawModel(model,
        //	  0, 0, 0,                  // translation
          //    0, x, 90);
		app.renderer.printString({50, 50},0, "hello world!");
		app.renderer.endFrame();
	}
	delete app.renderer.fontData;
	return 0;
}
