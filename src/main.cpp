#include "app.hpp"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine/filesystem.hpp"
#include "engine/timer.hpp"
#include "engine/renderer.hpp"

#include "engine/psx/irq.hpp"
#include "engine/psx/cd.hpp"

APP g_app;

#include "scenes/test.hpp"

int main(void) {

#ifdef PLATFORM_PSX
	initSerialIO(115200);
	ENGINE::PSX::initIRQ();
	ENGINE::PSX::g_CDInstance.provide( &ENGINE::PSX::CDRom::instance());
#endif
	ENGINE::g_fileSystemInstance.provide( &ENGINE::FileSystem::instance()); //must be done after initializing cd drive

	ENGINE::g_timerInstance.provide( &ENGINE::Timer::instance());
	ENGINE::g_rendererInstance.provide( &ENGINE::Renderer::instance());

    g_app.curscene.reset(new TestSCN());
	while(1) {
		ENGINE::g_rendererInstance.get()->beginFrame();
		
		assert(g_app.curscene);
     
        g_app.curscene->update();  
        g_app.curscene->draw();  
//		printf("time %llu\n", ENGINE::g_timerInstance.get()->getMS());		
//		printf("fps%d\n", ENGINE::g_rendererInstance.get()->getFPS());
#ifdef PLATFORM_PSX
//		g_app.renderer.printStringf({5, 5}, 0, "Heap usage: %zu/%zu bytes", getHeapUsage(), _heapLimit-_heapEnd);
//		printf("Heap usage: %zu/%zu bytes\n", getHeapUsage(), _heapLimit-_heapEnd);
#endif
		ENGINE::g_rendererInstance.get()->endFrame();
	}
	return 0;
}
