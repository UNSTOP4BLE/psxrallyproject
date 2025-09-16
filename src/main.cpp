#include "app.hpp"
#include "pc/gpugl.hpp"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#ifdef PLATFORM_PSX
#include "ps1/registers.h"
#endif

#include "scenes/test.hpp"

App g_app;

int main(int argc, const char **argv) {

#ifdef PLATFORM_PSX
	initSerialIO(115200);
#endif
	//g_app.fileprovider.init();

	int mode = 0;
#ifdef PLATFORM_PSX
	if ((GPU_GP1 & GP1_STAT_FB_MODE_BITMASK) == GP1_STAT_FB_MODE_PAL) {
		puts("Using PAL mode");
		mode = GP1_MODE_PAL;
	} else {
		puts("Using NTSC mode");
		mode = GP1_MODE_NTSC;
	}
	GTE::setupGTE();
#endif
	g_app.renderer.init(mode);
	

//	ENGINE::FS::File* file = g_app.fileprovider.openFile("font.xtex");
//	GFX::uploadTexture(g_app.renderer.fonttex, file->fdata);
//	file->close();

//	file = g_app.fileprovider.openFile("font.xfnt");
//	g_app.renderer.fontmap = GFX::loadFontMap(file->fdata);
//	file->close();

	ENGINE::SCENE::set(new TestSCN());

	while(1) {
		g_app.renderer.beginFrame();
		
		assert(g_app.curscene);
		g_app.renderer.drawRect({32, 32, 64, 64}, 0, gp0_rgb(255, 0, 0));
        g_app.curscene->update();  
        g_app.curscene->draw();  


//		g_app.renderer.printStringf({5, 5}, 0, "Heap usage: %zu/%zu bytes", getHeapUsage(), _heapLimit-_heapEnd);

		g_app.renderer.endFrame();
	}
	return 0;
}
