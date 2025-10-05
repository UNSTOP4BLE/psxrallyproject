#pragma once 

#include <stdint.h>
#include "common.hpp"

namespace ENGINE {

class Timer {
public:
    virtual uint64_t getMS(void) {return 0;}

    static Timer &instance();

protected:
    Timer() {}
};

extern COMMON::ServiceLocator<Timer> timerInstance;

//psx
namespace PSX {
class PSXTimer : public Timer {
public:
    uint32_t t2irqcount;

    PSXTimer(void);
    uint64_t getMS(void);
private:
    uint64_t getT2_value(void); 

};
}

}