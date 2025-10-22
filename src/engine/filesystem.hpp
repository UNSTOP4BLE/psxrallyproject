#pragma once 

#include "templates.hpp"
#include "psx/cd.hpp"

namespace ENGINE {

class FileSystem {
public:
    virtual uint8_t *readFile(const char *filename) {}

    static FileSystem &instance();
protected:
    FileSystem() {}
};

extern TEMPLATES::ServiceLocator<FileSystem> g_fileSystemInstance;

//psx
namespace PSX {

class PSXFileSystem : public FileSystem {
public:
	PSXFileSystem(void);	

    uint8_t *readFile(const char *filename);
private:
};
} 

}