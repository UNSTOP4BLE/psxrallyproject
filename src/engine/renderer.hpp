#pragma once 

#include "common.hpp"

namespace ENGINE {

class Renderer {
public:
	virtual void beginFrame(void) {}
	virtual void endFrame(void) {}

	virtual void drawRect(ENGINE::COMMON::RECT32 rect, int z, uint32_t col) {}
//	virtual void drawTexRect(const TextureInfo &tex, ENGINE::COMMON::RECT32 pos, int z, int col) {}
  //  virtual void drawTexQuad(const TextureInfo &tex, ENGINE::COMMON::RECT32 pos, int z, uint32_t col) {}
//	virtual void drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot) {}
	virtual void printString(ENGINE::COMMON::XY32 pos, int z, const char *str) {}
    virtual void printStringf(ENGINE::COMMON::XY32 pos, int z, const char *fmt, ...) {}
    
    void setClearCol(uint8_t r, uint8_t g, uint8_t b) {
//        clearcol = gp0_rgb(r, g, b);
    }
	uint32_t getFPS(void) {return fps;}

    static Renderer &instance();

protected:
	uint32_t scrw, scrh;
    uint32_t refreshrate, fps;
    uint32_t vsynccounter, framecounter;
    uint32_t clearcol;
    Renderer() {}
};

extern TEMPLATES::ServiceLocator<Renderer> g_rendererInstance;

//psx
namespace PSX {
    
struct DMAChain {
	uint32_t data[ENGINE::COMMON::CHAIN_BUFFER_SIZE];
	uint32_t orderingtable[ENGINE::COMMON::ORDERING_TABLE_SIZE];
	uint32_t *nextpacket;
};

class PSXRenderer : public Renderer {
public:
	PSXRenderer(void);
	
	void beginFrame(void);
	void endFrame(void);

	void drawRect(ENGINE::COMMON::RECT32 rect, int z, uint32_t col);
//	void drawTexRect(const TextureInfo &tex, ENGINE::COMMON::RECT32 pos, int z, int col);
 //   void drawTexQuad(const TextureInfo &tex, ENGINE::COMMON::RECT32 pos, int z, uint32_t col);
	//void drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot);
	void printString(ENGINE::COMMON::XY32 pos, int z, const char *str);
    void printStringf(ENGINE::COMMON::XY32 pos, int z, const char *fmt, ...);
	void handleVSyncInterrupt(void);
private:
	bool usingsecondframe;
	DMAChain dmachains[2];

	void waitForVSync(void);
	uint32_t *allocatePacket(int z, int numcommands);

	DMAChain *getCurrentChain(void) {
		return &dmachains[usingsecondframe];
	}

};
} 

}