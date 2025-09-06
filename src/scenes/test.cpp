#include "test.hpp"

extern const uint8_t g_mymodel[];
int x = 0;
TestSCN::TestSCN(void) {
    carmodel = GFX::loadModel(g_mymodel); 
}

void TestSCN::update(void) {
}

void TestSCN::draw(void) {
    x += 5;
    g_app.renderer.drawModel(carmodel, {0, 32, 0}, {0, x, 90+2048});
}

TestSCN::~TestSCN(void) {

}