
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
#include "ps1/gte.h"
#include "ps1/registers.h"


#define NUM_CUBE_VERTICES 8
#define NUM_CUBE_FACES    6

//spicy texture
extern const uint8_t textureData[];

/*
static const GTEVector16 cubeVertices[NUM_CUBE_VERTICES] = {
	{ .x = -32, .y = -32, .z = -32 },
	{ .x =  32, .y = -32, .z = -32 },
	{ .x = -32, .y =  32, .z = -32 },
	{ .x =  32, .y =  32, .z = -32 },
	{ .x = -32, .y = -32, .z =  32 },
	{ .x =  32, .y = -32, .z =  32 },
	{ .x = -32, .y =  32, .z =  32 },
	{ .x =  32, .y =  32, .z =  32 }
};

// Note that there are several requirements on the order of vertices:
// - they must be arranged in a Z-like shape rather than clockwise or
//   counterclockwise, since the GPU processes a quad with vertices (A, B, C, D)
//   as two triangles with vertices (A, B, C) and (B, C, D) respectively;
// - the first 3 vertices must be ordered clockwise when the face is viewed from
//   the front, as the code relies on this to determine whether or not the quad
//   is facing the camera (see main()).
// For instance, only the first of these faces (viewed from the front) has its
// vertices ordered correctly:
//     0----1        0----1        2----3
//     |  / |        | \/ |        | \  |
//     | /  |        | /\ |        |  \ |
//     2----3        3----2        0----1
//     Correct    Not Z-shaped  Not clockwise
static const Face cubeFaces[NUM_CUBE_FACES] = {
	{ .vertices = { 0, 1, 2, 3 }, .color = 0x0000ff },
	{ .vertices = { 6, 7, 4, 5 }, .color = 0x00ff00 },
	{ .vertices = { 4, 5, 0, 1 }, .color = 0x00ffff },
	{ .vertices = { 7, 6, 3, 2 }, .color = 0xff0000 },
	{ .vertices = { 6, 4, 2, 0 }, .color = 0xff00ff },
	{ .vertices = { 5, 7, 1, 3 }, .color = 0xffff00 }
};
*/
int main(int argc, const char **argv) {
	initSerialIO(115200);

	Renderer renderer;

	if ((GPU_GP1 & GP1_STAT_FB_MODE_BITMASK) == GP1_STAT_FB_MODE_PAL) {
		puts("Using PAL mode");
		renderer.init(GP1_MODE_PAL, SCREEN_WIDTH, SCREEN_HEIGHT);
	} else {
		puts("Using NTSC mode");
		renderer.init(GP1_MODE_NTSC, SCREEN_WIDTH, SCREEN_HEIGHT);
	}

	setupGTE(SCREEN_WIDTH, SCREEN_HEIGHT);


	// Load the texture, placing it next to the two framebuffers in VRAM.
	TextureInfo texture;

	renderer.uploadTexture(
		texture,
		textureData,
		SCREEN_WIDTH * 2,
		0,
		32,
		32
	);

	while(1) {
		renderer.clear();
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

		renderer.drawRect({10, 10, 20, 20}, 128, 0, 0);
		renderer.drawRect({10, 20+20, 20, 20},   0, 128, 0);
		renderer.drawRect({10, 50+20, 20, 20},   0,  0, 128);
		renderer.drawTexRect(texture, {40, 10});
		renderer.drawTexRect(texture, {40, 20+32});
		
		renderer.flip();

	}

	return 0;
}
