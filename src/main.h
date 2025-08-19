#pragma once

#include "gpu.h"
#include "system.h"

static void handleInterrupt(void* arg0, void* arg1);

class App {
public:
	GFX::Renderer renderer;
	
	void setupInterrupts(void);
	void handleCDROMInterrupt(void);

};
