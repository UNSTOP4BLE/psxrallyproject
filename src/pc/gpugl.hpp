#pragma once

#include "../gpu.hpp"
#include "../fixed.hpp"
#include <glad/glad.h>    
#include <GLFW/glfw3.h>

inline uint32_t gp0_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xFF) << 0)
         | ((g & 0xFF) << 8)
         | ((b & 0xFF) << 16);
}

namespace GFX {

class GLRenderer : public Renderer {
public:
	void init(int mode);
	
	void beginFrame(void);
	void endFrame(void);

	void drawRect(RECT<int32_t> rect, int z, uint32_t col);
	void drawTexRect(const TextureInfo &tex, XY<int32_t> pos, int z, int col);
    void drawTexQuad(const TextureInfo &tex, RECT<int32_t> pos, int z, uint32_t col);
	void drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot);
	void printString(XY<int32_t> pos, int z, const char *str);
    void printStringf(XY<int32_t> pos, int z, const char *fmt, ...);

    void setClearCol(uint8_t r, uint8_t g, uint8_t b) {
        clearcol = gp0_rgb(r, g, b);
    }
	TextureInfo fonttex;
	FontData *fontmap;
private:
    GLFWwindow* window;
    GLuint prog;
    GLuint vao, vbo;
	int framecounter;
    uint32_t clearcol;

};

void uploadTexture(TextureInfo &info, const void *image);
Model *loadModel(const uint8_t *data);
void freeModel(Model *model);
FontData *loadFontMap(const uint8_t *data);

}