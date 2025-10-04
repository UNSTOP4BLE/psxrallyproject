#include "app.hpp"
#include <assert.h>
#include <stdio.h>

APP g_app;

int main(int argc, const char **argv) {
#ifdef PLATFORM_PSX
	initSerialIO(115200);
#endif

//	ENGINE::SCENE::set(new TestSCN());

	while(1) {
//		g_app.renderer.beginFrame();
		
		assert(g_app.curscene);
     
        g_app.curscene->update();  
        g_app.curscene->draw();  

#ifdef PLATFORM_PSX
//		g_app.renderer.printStringf({5, 5}, 0, "Heap usage: %zu/%zu bytes", getHeapUsage(), _heapLimit-_heapEnd);
#endif
//		g_app.renderer.endFrame();
	}
	return 0;
}
