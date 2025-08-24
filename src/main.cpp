#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "gpu.h"
#include "gte.h"
#include "ps1/gpucmd.h"
#include "ps1/registers.h"

//spicy texture
extern const uint8_t textureData[];
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

	// Load the texture, placing it next to the two framebuffers in VRAM.
	GFX::TextureInfo texture;
	GFX::uploadTexture(texture, textureData, {SCREEN_WIDTH * 2, 0, 32, 32});

    auto model = GFX::loadModel(mymodel);
	int x = 0;
	while(1) {
		renderer.beginFrame();

		GFX::XY screen[4] = { {0,0}, {32,0}, {0,32}, {32,32} }; // TL, TR, BL, BR
		GFX::XY uv[4]     = { {0,0}, {32,0}, {0,32}, {32,32} }; // TL, TR, BL, BR


//		renderer.drawTri(screen[0],screen[1],screen[2], 0, gp0_rgb(255, 128, 128));

		renderer.drawTexQuad(texture, screen[0], screen[1], screen[2], screen[3],
								uv[0], uv[1], uv[2], uv[3], 0, gp0_rgb(128, 128, 128));

		x+= 10;
    	renderer.drawModel(model,
        	  0, 0, 0,                  // translation
              0, x, 90, texture);

		renderer.endFrame();
	}

	return 0;
}
