#pragma once 

#include "gpu.hpp"
#include "engine/scene.hpp"

class App {
public:
	GFX::Renderer renderer;
	ENGINE::SCENE::Scene *curscene;
};

extern App g_app;