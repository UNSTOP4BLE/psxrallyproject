#pragma once

#include "../scene.hpp"

class TestSCN : public SCENE::Scene {
public:
    TestSCN(void);
    void update(void);
    void draw(void);
    ~TestSCN(void); 
private:
};