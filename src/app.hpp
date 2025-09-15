#pragma once 

#include "engine/scene.hpp"
#include "engine/fs/host.hpp"
#ifdef PLATFORM_PSX
#include "psx/gpupsx.hpp"
#else
#include "pc/gpugl.hpp"
#endif
class App {
public:
#ifdef PLATFORM_PSX
	GFX::PSXRenderer renderer;
#else
	GFX::GLRenderer renderer;
#endif

	ENGINE::SCENE::Scene *curscene;
	//ENGINE::FS::HostProvider fileprovider;
};

extern App g_app;