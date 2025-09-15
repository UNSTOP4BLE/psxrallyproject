#include "gpugl.hpp"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

namespace GFX {
void GLRenderer::init(int mode) {
	usingsecondframe = false;
	framecounter = 0;
	setClearCol(64,64,64);
}

void GLRenderer::beginFrame(void) {
}

void GLRenderer::endFrame(void) {

}

void GLRenderer::drawRect(RECT<int32_t> rect, int z, uint32_t col) {
    
}

void GLRenderer::drawTexRect(const TextureInfo &tex, XY<int32_t> pos, int z, int col) {

}

void GLRenderer::drawTexQuad(const TextureInfo &tex, RECT<int32_t> pos, int z, uint32_t col) {
 
}

void GLRenderer::drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot) {
	
}

void GLRenderer::printString(XY<int32_t> pos, int z, const char *str) {
	assert(fontmap);
}

void GLRenderer::printStringf(XY<int32_t> pos, int z, const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    printString(pos, z, buf);
}
}