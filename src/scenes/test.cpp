#include "test.hpp"

int testx = 0;
TestSCN::TestSCN(void) {
    //todo improve file loading so i dont need to allocate a ton of crap
    car.init("cars/impreza555/impreza555.xmdl");
}

void TestSCN::update(void) {
    testx += 10;
    car.rot = {0, testx, 45};
    car.update();
}

void TestSCN::draw(void) {
    car.render();
}

TestSCN::~TestSCN(void) {
    car.free();
}