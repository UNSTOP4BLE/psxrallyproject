#pragma once 

#include "templates.hpp"

namespace ENGINE {

    class Audio {
    public:
        static Audio &instance();
    protected:
        Audio() {}
    };

    extern TEMPLATES::ServiceLocator<Audio> g_audioInstance;

    //psx
#ifdef PLATFORM_PSX
    namespace PSX {
        class PSXAudio : public Audio {
        public:
            PSXAudio(void);	
        private:
        };
    } 
#else
    namespace GENERIC {
        
    }
#endif
}