#pragma once 

#include "templates.hpp"
//#include "psx/audio.hpp"

namespace ENGINE {

    class Audio {
    public:
        static Audio &instance();
    protected:
        Audio() {}
    };

    extern TEMPLATES::ServiceLocator<Audio> g_audioInstance;

    //psx
    namespace PSX {
        class PSXAudio : public Audio {
        public:
            PSXAudio(void);	
        private:
        };
    } 

}