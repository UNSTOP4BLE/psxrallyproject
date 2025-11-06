#include "test.hpp"
#include "../engine/psx/cd.hpp"
#include "../engine/filesystem.hpp"
#include <stdio.h>
//int testx = 0;
TestSCN::TestSCN(void) {
    //todo improve file loading so i dont need to allocate a ton of crap
  //  car.init("cars/impreza555/impreza555.xmdl");
    ENGINE::PSX::PSXFile f;
    f.open("CARS/IMPREZA555;1");
    printf("File size = %u bytes\n", f.size);

    uint8_t buf[f.size];

} 

void TestSCN::update(void) {
    //testx += 10;
    //car.rot = {0, testx, 45};
  //  car.update();
}

void TestSCN::draw(void) {
    //car.render();
}

TestSCN::~TestSCN(void) {
    //car.free();
}