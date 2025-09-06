#pragma once 

#include "gpu.hpp"
#include "scene.hpp"

class App {
public:
	GFX::Renderer renderer;
	SCENE::Scene *curscene;
};

extern App g_app;