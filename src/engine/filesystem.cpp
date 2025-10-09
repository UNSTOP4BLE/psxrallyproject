#include "filesystem.hpp"

namespace ENGINE {

TEMPLATES::ServiceLocator<FileSystem> g_fileSystemInstance;


FileSystem &FileSystem::instance() {
    static FileSystem *instance;
    #ifdef PLATFORM_PSX
    instance = new PSX::PSXFileSystem();
    #endif
    return *instance;
}

}