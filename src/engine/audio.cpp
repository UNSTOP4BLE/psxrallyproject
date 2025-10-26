#include "audio.hpp"

namespace ENGINE {

    TEMPLATES::ServiceLocator<Audio> g_audioInstance;

    Audio &Audio::instance() {
        static Audio *instance;
        #ifdef PLATFORM_PSX
    //    instance = new PSX::PSXAudio();
        #endif
        return *instance;
    }

}