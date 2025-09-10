#include "test.hpp"

int testx = 0;
TestSCN::TestSCN(void) {
    car.init("blablatest");
}

void TestSCN::update(void) {
    testx += 10;
    car.setRot({0, testx, 45});
}

void TestSCN::draw(void) {
    car.render();
}

TestSCN::~TestSCN(void) {

}