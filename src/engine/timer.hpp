#pragma once 

#include <stdint.h>
#include "templates.hpp"

namespace ENGINE {

    class Timer {
    public:
        virtual uint64_t getMS(void) {return 0;}

        static Timer &instance();

    protected:
        Timer() {}
    };

    extern TEMPLATES::ServiceLocator<Timer> g_timerInstance;

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

    namespace GENERIC {
        class ChronoTimer : public Timer {
        public:
            ChronoTimer(void);
            uint64_t getMS(void);
        private:
        };
    }

}