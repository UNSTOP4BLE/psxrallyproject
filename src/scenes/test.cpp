#include "test.hpp"
#include "../engine/psx/cd.hpp"
#include "../engine/filesystem.hpp"
#include <stdio.h>
//int testx = 0;
TestSCN::TestSCN(void) {
    //todo improve file loading so i dont need to allocate a ton of crap
  //  car.init("cars/impreza555/impreza555.xmdl");
    ENGINE::PSX::PSXFile f;
    f.open("DIR1/DIR2/FILE1;1");
    
    uint32_t size = f.size;
    printf("fsize %u\n", size);
    auto buffer = new uint8_t[size];
    f.read(buffer, size);
    ENGINE::COMMON::hexDump(buffer, size);
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