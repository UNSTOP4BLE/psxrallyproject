#include "../filesystem.hpp"
#include <stdint.h>
#include <assert.h>

namespace ENGINE::GENERIC {

    //file
    uint32_t GenericFile::read(void *output, uint32_t length) {  
        return fread(output, 1, length, _handle);
    }

    uint64_t GenericFile::seek(uint64_t offset) {
        if (!_handle) return 0;

        fseek(_handle, offset, SEEK_SET);
        return ftell(_handle);
    }


    GenericFileSystem::GenericFileSystem(void) {
        //nothing to initialize
    }

    File *GenericFileSystem::findFile(const char *path) {
        GenericFile *file = new GenericFile();
        assert(file);
        file->_handle = fopen(path, "rb");
        assert(file->_handle);

        //set filesize
        fseek(file->_handle, 0, SEEK_END);
        file->_size = ftell(file->_handle);
        fseek(file->_handle, 0, SEEK_SET);

        return file;
    }

} 