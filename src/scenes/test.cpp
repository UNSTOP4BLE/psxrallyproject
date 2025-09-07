#include "test.hpp"

int testx = 0;
TestSCN::TestSCN(void) {
    car.init("blablatest");
}

void TestSCN::update(void) {
}

void TestSCN::draw(void) {
    testx += 10;
    car.render();
}

TestSCN::~TestSCN(void) {

}