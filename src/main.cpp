
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

#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "gpu.h"
#include "gte.h"
#include "ps1/gpucmd.h"
#include "ps1/registers.h"

App app;

//spicy texture
extern const uint8_t textureData[];

#define NUM_CUBE_VERTICES 8
#define NUM_CUBE_FACES    12

// Cube vertices (same as before)
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


static void handleInterrupt(void* arg0, void* arg1) {
	auto app = reinterpret_cast<App *>(arg0);

	if (acknowledgeInterrupt(IRQ_CDROM))
		app->handleCDROMInterrupt();
}

void App::setupInterrupts(void) {
	setInterruptHandler(handleInterrupt, this, nullptr);

	IRQ_MASK = (1 << IRQ_CDROM); // enable the cdrom irq
	cop0_enableInterrupts();
}

void App::handleCDROMInterrupt(void) {
	// ...
}

int main(int argc, const char **argv) {
	initSerialIO(115200);
	installExceptionHandler();

	if ((GPU_GP1 & GP1_STAT_FB_MODE_BITMASK) == GP1_STAT_FB_MODE_PAL) {
		puts("Using PAL mode");
		app.renderer.init(GP1_MODE_PAL, SCREEN_WIDTH, SCREEN_HEIGHT);
	} else {
		puts("Using NTSC mode");
		app.renderer.init(GP1_MODE_NTSC, SCREEN_WIDTH, SCREEN_HEIGHT);
	}

	GTE::setupGTE(SCREEN_WIDTH, SCREEN_HEIGHT);


	// Load the texture, placing it next to the two framebuffers in VRAM.
	GFX::TextureInfo texture;

	GFX::uploadTexture(texture, textureData, {SCREEN_WIDTH * 2, 0, 32, 32});
	GFX::Face cubeFaces[NUM_CUBE_FACES] = {
    // Back (red, flat)
    { .vertices={0,1,2}, .color=gp0_rgb(255,0,0), .textured=false },
    { .vertices={2,1,3}, .color=gp0_rgb(255,0,0), .textured=false },

    // Front (textured)
    { .vertices={6,7,4}, .u={0,32,0}, .v={0,0,32},
      .texInfo=texture,
      .textured=true
    },
    { .vertices={4,7,5}, .u={0,32,32}, .v={32,0,32},
      .texInfo=texture,
      .textured=true
    },

    // Bottom (blue, flat)
    { .vertices={4,5,0}, .color=gp0_rgb(0,0,255), .textured=false },
    { .vertices={0,5,1}, .color=gp0_rgb(0,0,255), .textured=false },

    // Top (yellow, flat)
    { .vertices={7,6,3}, .color=gp0_rgb(255,255,0), .textured=false },
    { .vertices={3,6,2}, .color=gp0_rgb(255,255,0), .textured=false },

    // Left (magenta, flat)
    { .vertices={6,4,2}, .color=gp0_rgb(255,0,255), .textured=false },
    { .vertices={2,4,0}, .color=gp0_rgb(255,0,255), .textured=false },

    // Right (cyan, flat)
    { .vertices={5,7,1}, .color=gp0_rgb(0,255,255), .textured=false },
    { .vertices={1,7,3}, .color=gp0_rgb(0,255,255), .textured=false }
};


	GFX::Model cubeModel = {
		.vertices    = cubeVertices,
		.numVertices = NUM_CUBE_VERTICES,
		.faces       = cubeFaces,
		.numFaces    = NUM_CUBE_FACES
	};
	int x = 0;
	while(1) {
		app.renderer.beginFrame();
		// Reset the GTE's translation vector (added to each vertex) and
		// transformation matrix, then modify the matrix to rotate the cube. The
		// translation vector is used here to move the cube away from the camera
		// so it can be seen.
		/*
		gte_setControlReg(GTE_TRX,   0);
		gte_setControlReg(GTE_TRY,   0);
		gte_setControlReg(GTE_TRZ, 128);
		
		gte_setRotationMatrix(
			ONE,   0,   0,
			  0, ONE,   0,
			  0,   0, ONE
		);

		rotateCurrentMatrix(0, frameCounter * 16, frameCounter * 12);
		frameCounter++;

		// Draw the cube one face at a time.
		for (int i = 0; i < NUM_CUBE_FACES; i++) {
			const Face *face = &cubeFaces[i];

			// Apply perspective projection to the first 3 vertices. The GTE can
			// only process up to 3 vertices at a time, so we'll transform the
			// last one separately.
			gte_loadV0(&cubeVertices[face->vertices[0]]);
			gte_loadV1(&cubeVertices[face->vertices[1]]);
			gte_loadV2(&cubeVertices[face->vertices[2]]);
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

			gte_loadV0(&cubeVertices[face->vertices[3]]);
			gte_command(GTE_CMD_RTPS | GTE_SF);

			// Calculate the average Z coordinate of all vertices and use it to
			// determine the ordering table bucket index for this face.
			gte_command(GTE_CMD_AVSZ4 | GTE_SF);
			int zIndex = gte_getDataReg(GTE_OTZ);

			if ((zIndex < 0) || (zIndex >= ORDERING_TABLE_SIZE))
				continue;

			// Create a new quad and give its vertices the X/Y coordinates
			// calculated by the GTE.
			ptr    = allocatePacket(chain, zIndex, 5);
			ptr[0] = face->color | gp0_shadedQuad(false, false, false);
			ptr[1] = xy0;
			gte_storeDataReg(GTE_SXY0, 2 * 4, ptr);
			gte_storeDataReg(GTE_SXY1, 3 * 4, ptr);
			gte_storeDataReg(GTE_SXY2, 4 * 4, ptr);
		}

		ptr    = allocatePacket(chain, ORDERING_TABLE_SIZE - 1, 3);
		ptr[0] = gp0_rgb(64, 64, 64) | gp0_vramFill();
		ptr[1] = gp0_xy(bufferX, bufferY);
		ptr[2] = gp0_xy(SCREEN_WIDTH, SCREEN_HEIGHT);

		ptr    = allocatePacket(chain, ORDERING_TABLE_SIZE - 1, 4);
		ptr[0] = gp0_texpage(0, true, false);
		ptr[1] = gp0_fbOffset1(bufferX, bufferY);
		ptr[2] = gp0_fbOffset2(
			bufferX + SCREEN_WIDTH  - 1,
			bufferY + SCREEN_HEIGHT - 2
		);
		ptr[3] = gp0_fbOrigin(bufferX, bufferY);
		*/



		//spicy
		// Use the texture we uploaded to draw a sprite (textured rectangle).
		// Two separate commands have to be sent: a texpage command to apply our
		// texpage attribute and disable dithering, followed by the actual
		// rectangle drawing command.

		/*
*/
GFX::XY screen[4] = { {100,50}, {132,50}, {132,82}, {100,82} }; // four corners
GFX::XY uv[4]     = { {0,0}, {32,0}, {32,32}, {0,32} };          // texture coords

		x+= 10;
//    	app.renderer.drawModel(&cubeModel,
  //      	  0, 0, 0,                  // translation
    //          0, 90+x, 90);


// Z / ordering table index
int zIndex = 10;

// Color modulation
uint32_t col = 0xFFFFFF;
app.renderer.drawTexQuad(
    texture,
    screen[0], screen[1], screen[2], screen[3],   // 4 screen vertices
    uv[0], uv[1], uv[2], uv[3],                   // 4 UV coords
    0,                                            // zIndex (OT slot)
    gp0_rgb(128,128,128)                          // modulation color
);

		app.renderer.endFrame();
	}

	return 0;
}
