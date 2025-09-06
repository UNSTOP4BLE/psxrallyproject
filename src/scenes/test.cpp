#include "test.hpp"

#include "game.hpp"

TestSCN::TestSCN(void) {
}
int x = 0;
void TestSCN::update(void) {
    x++;
    if (x > 100)
        SCENE::set(new GameSCN());
}

void TestSCN::draw(void) {
}

TestSCN::~TestSCN(void) {

}