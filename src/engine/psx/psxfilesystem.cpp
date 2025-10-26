#include "../filesystem.hpp"
#include <assert.h>
#include <stdio.h>
#include <string.h>

namespace ENGINE::PSX {
    const ISO9660::Directory* readDir(const ISO9660::Directory *curdir, const char *dir) {
        uint32_t dir_lba = curdir->lba.le;
        size_t dir_size = curdir->datalength.le;
        size_t dir_num_sectors = (dir_size + ENGINE::PSX::SECTOR_SIZE - 1) / ENGINE::PSX::SECTOR_SIZE;

        uint8_t *dir_buffer = new uint8_t[dir_num_sectors * ENGINE::PSX::SECTOR_SIZE];
        ENGINE::PSX::g_CDInstance.get()->startRead(
            dir_lba,
            dir_buffer,
            dir_num_sectors,
            true,
            true
        );

        uint8_t* ptr = dir_buffer;
        uint8_t* end = dir_buffer + dir_size;
        while (ptr < end) {
            if (ptr[0] == 0) {
                ptr++;
                continue;
            }

            auto entry = reinterpret_cast<ISO9660::Directory*>(ptr);
            const char* name = reinterpret_cast<const char*>(ptr + sizeof(ISO9660::Directory));
            uint8_t namelen = entry->filename_length;

            //skip special entries "." and ".."
            if (namelen == 1 && (name[0] == '\0' || name[0] == '\1')) {
                ptr += entry->length;
                continue;
            }

            //found dir
            if (strncmp(dir, name, namelen) == 0) {
                delete[] dir_buffer;
                return entry;
            }

            ptr += entry->length;
        }

        delete[] dir_buffer;
        return nullptr;
    }


    PSXFileSystem::PSXFileSystem(void) {
        //pvd sector
        g_CDInstance.get()->startRead(16, &pvd, sizeof(pvd) / ENGINE::PSX::SECTOR_SIZE, true, true); 
        //assert(pvd.magic == "CD001"_c); //todo _c operator
        printf("build from %s\n", pvd.creation_date);

        rootdir = reinterpret_cast<const ISO9660::Directory*>(&pvd.rootdir);
    }
    

} 