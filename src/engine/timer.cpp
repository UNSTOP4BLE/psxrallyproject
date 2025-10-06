#include "timer.hpp"

namespace ENGINE {

COMMON::ServiceLocator<Timer> g_timerInstance;

Timer &Timer::instance() {
    static Timer *instance;
    #ifdef PLATFORM_PSX
    instance = new PSX::PSXTimer();
    #endif
    return *instance;
}

}