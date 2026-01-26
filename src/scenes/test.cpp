#include "test.hpp"
#include "../engine/filesystem.hpp"
#include <stdio.h>

//int testx = 0;
TestSCN::TestSCN(void) {
    //todo improve file loading so i dont need to allocate a ton of crap
  //  car.init("cars/impreza555/impreza555.xmdl");<
//    ENGINE::File *f = ENGINE::g_fileSystemInstance.get()->findFile("DIR3/FILE2");
  //  f.open("CARS/IMPREZA555;1");
//    printf("File size = %llu bytes\n", f->getSize());
  //  uint8_t buf[16];
    //auto read = f->read(buf, 16);
    //printf("read returned %u\n", read);
    //uint8_t buf[f.size];

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