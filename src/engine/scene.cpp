#include "scene.hpp"
#include "../app.hpp"

namespace ENGINE::SCENE {

void set(Scene *scn) {   
    if (g_app.curscene != NULL)
        delete g_app.curscene;
    g_app.curscene = scn;
}

}