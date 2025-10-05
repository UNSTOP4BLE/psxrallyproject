#pragma once

#include "../app.hpp"

class TestSCN : public ENGINE::COMMON::Scene {
public:
    TestSCN(void);
    void update(void);
    void draw(void);
    ~TestSCN(void); 
private:
//    ENGINE::Object3D car;
};