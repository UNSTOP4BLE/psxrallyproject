#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "gpu.h"
#include "gte.h"
#include "ps1/gpucmd.h"
#include "ps1/registers.h"

extern const uint8_t mymodel[];

int main(int argc, const char **argv) {
	initSerialIO(115200);

	GFX::Renderer renderer;

	if ((GPU_GP1 & GP1_STAT_FB_MODE_BITMASK) == GP1_STAT_FB_MODE_PAL) {
		puts("Using PAL mode");
		renderer.init(GP1_MODE_PAL, SCREEN_WIDTH, SCREEN_HEIGHT);
	} else {
		puts("Using NTSC mode");
		renderer.init(GP1_MODE_NTSC, SCREEN_WIDTH, SCREEN_HEIGHT);
	}

	GTE::setupGTE(SCREEN_WIDTH, SCREEN_HEIGHT);

    auto model = GFX::loadModel(mymodel);
	int x = 0;
	while(1) {
		renderer.beginFrame();
		
		x += 10;
    	renderer.drawModel(model,
        	  0, 0, 0,                  // translation
              0, x, 90);

		renderer.endFrame();
	}

	return 0;
}
