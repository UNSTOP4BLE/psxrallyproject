#pragma once

#include "../app.hpp"

class TestSCN : public SCENE::Scene {
public:
    TestSCN(void);
    void update(void);
    void draw(void);
    ~TestSCN(void); 
private:
    const GFX::ModelFile* carmodel;
};