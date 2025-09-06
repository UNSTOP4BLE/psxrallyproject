#pragma once

#include "../scene.hpp"

class GameSCN : public SCENE::Scene {
public:
    GameSCN(void);
    void update(void);
    void draw(void);
    ~GameSCN(void); 
private:
};