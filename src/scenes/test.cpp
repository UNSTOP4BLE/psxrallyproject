#include "test.hpp"

extern const uint8_t g_mymodel[];

TestSCN::TestSCN(void) {
    carmodel = GFX::loadModel(g_mymodel); 
}

void TestSCN::update(void) {
}

void TestSCN::draw(void) {
    g_app.renderer.drawModel(carmodel, {0, 0, 0}, {0, 0, 0});
}

TestSCN::~TestSCN(void) {

}