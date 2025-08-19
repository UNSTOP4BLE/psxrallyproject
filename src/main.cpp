
/*
 * Having explored the capabilities of the PS1's GPU in previous examples, it is
 * now time to focus on the other piece of hardware that makes 3D graphics on
 * the PS1 possible: the geometry transformation engine (GTE), a specialized
 * coprocessor whose job is to perform various geometry-related calculations
 * much faster than the CPU could on its own. To draw a 3D scene the CPU can use
 * the GTE to calculate the screen space coordinates of each polygon's vertices,
 * then pack those into a display list which will be sent off to the GPU for
 * drawing. In this example we're going to draw a spinning model of a cube,
 * using the GTE to carry out the computationally heavy tasks of rotation and
 * perspective projection.
 *
 * Unlike any other peripheral on the console, the GTE is not memory-mapped
 * but rather accessed through special CPU instructions that require the use of
 * inline assembly. This tutorial will thus use the cop0.h and gte.h headers I
 * wrote to abstract away the low-level assembly required to access GTE
 * registers, focusing on its practical usage instead. This example may be
 * harder to follow compared to previous ones for people unfamiliar with basic
 * linear algebra and 3D geometry concepts, so familiarizing with those is
 * highly recommended.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "gpu.h"
#include "gte.h"
#include "ps1/gpucmd.h"
#include "ps1/registers.h"


#define NUM_CUBE_VERTICES 8
#define NUM_CUBE_FACES    12

//spicy texture
extern const uint8_t textureData[];

// 8 vertices of a cube, centered at origin
static const GTEVector16 cubeVertices[NUM_CUBE_VERTICES] = {
    { .x = -32, .y = -32, .z = -32 }, // 0
    { .x =  32, .y = -32, .z = -32 }, // 1
    { .x = -32, .y =  32, .z = -32 }, // 2
    { .x =  32, .y =  32, .z = -32 }, // 3
    { .x = -32, .y = -32, .z =  32 }, // 4
    { .x =  32, .y = -32, .z =  32 }, // 5
    { .x = -32, .y =  32, .z =  32 }, // 6
    { .x =  32, .y =  32, .z =  32 }  // 7
};

// 6 faces (each is a quad defined by 4 vertex indices)
// Colors are just arbitrary RGB values for testing.
static const GTE::Face cubeFaces[NUM_CUBE_FACES] = {
    { .vertices = { 0, 1, 2, 3 }, .color = gp0_rgb(255,   0,   0) }, // back (red)
    { .vertices = { 6, 7, 4, 5 }, .color = gp0_rgb(  0, 255,   0) }, // front (green)
    { .vertices = { 4, 5, 0, 1 }, .color = gp0_rgb(  0,   0, 255) }, // bottom (blue)
    { .vertices = { 7, 6, 3, 2 }, .color = gp0_rgb(255, 255,   0) }, // top (yellow)
    { .vertices = { 6, 4, 2, 0 }, .color = gp0_rgb(255,   0, 255) }, // left (magenta)
    { .vertices = { 5, 7, 1, 3 }, .color = gp0_rgb(  0, 255, 255) }  // right (cyan)
};

// Wrap it in a Model struct
static const GFX::Model cubeModel = {
    .vertices   = cubeVertices,
    .numVertices = NUM_CUBE_VERTICES,
    .faces      = cubeFaces,
    .numFaces   = NUM_CUBE_FACES
};

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

	int x = 0;
	while(1) {
		renderer.beginFrame();

		GFX::Pos screen[4] = { {100,50}, {132,50}, {132,82}, {100,82} }; // four corners
		GFX::Pos uv[4]     = { {0,0}, {32,0}, {32,32}, {0,32} };          // texture coords

		renderer.drawTexQuad(texture, screen[0], screen[1], screen[2], screen[3],
								uv[0], uv[1], uv[2], uv[3]);

		x+= 10;
    	renderer.drawModel(&cubeModel,
        	  0, 0, 0,                  // translation
              0, 90+x, 90);

		renderer.endFrame();
	}

	return 0;
}
