#include "app.hpp"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine/timer.hpp"
#include "engine/renderer.hpp"

#include "engine/psx/irq.hpp"


APP g_app;

#include "scenes/test.hpp"

int main(void) {

#ifdef PLATFORM_PSX
	initSerialIO(115200);
	ENGINE::PSX::initIRQ();
#endif
	ENGINE::timerInstance.provide( &ENGINE::Timer::instance());
	ENGINE::rendererInstance.provide( &ENGINE::Renderer::instance());

    g_app.curscene.reset(new TestSCN());
	while(1) {
		ENGINE::rendererInstance.get()->beginFrame();
		
		assert(g_app.curscene);
     
        g_app.curscene->update();  
        g_app.curscene->draw();  

		printf("time %llu\n", ENGINE::timerInstance.get()->getMS());
		
//		printf("fps%d\n", ENGINE::rendererInstance.get()->getFPS());
#ifdef PLATFORM_PSX
//		g_app.renderer.printStringf({5, 5}, 0, "Heap usage: %zu/%zu bytes", getHeapUsage(), _heapLimit-_heapEnd);
		printf("Heap usage: %zu/%zu bytes\n", getHeapUsage(), _heapLimit-_heapEnd);
#endif
		ENGINE::rendererInstance.get()->endFrame();
	}
	return 0;
}
