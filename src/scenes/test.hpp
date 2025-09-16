#pragma once

#include "../app.hpp"

#include "../engine/object.hpp"

class TestSCN : public ENGINE::SCENE::Scene {
public:
    TestSCN(void);
    void update(void);
    void draw(void);
    ~TestSCN(void); 
private:
    ENGINE::Object3D car;
};