#pragma once 

#include "gpu.hpp"
#include "engine/scene.hpp"
#include "engine/fs/host.hpp"

class App {
public:
	GFX::Renderer renderer;
	ENGINE::SCENE::Scene *curscene;
	ENGINE::FS::HostProvider fileprovider;
};

extern App g_app;