#include "../filesystem.hpp"
#include <assert.h>
#include <stdio.h>
#include <string.h>

namespace ENGINE::PSX {
   
    const ISO9660::Entry* readDir(const ISO9660::Entry *curdir, const char *dir) {
        if (!dir || !dir[0]) {
            return curdir;
        }

        //allocate buffer for current directory
        uint32_t dir_lba = curdir->lba.le;
        size_t dir_size = curdir->datalength.le;
        size_t dir_num_sectors = (dir_size + ENGINE::PSX::SECTOR_SIZE - 1) / ENGINE::PSX::SECTOR_SIZE;

        ENGINE::TEMPLATES::UniquePtr<uint8_t[]> dir_buffer(new uint8_t[dir_num_sectors * ENGINE::PSX::SECTOR_SIZE]);
        ENGINE::PSX::g_CDInstance.get()->startRead(
            dir_lba,
            dir_buffer.get(),
            dir_num_sectors,
            true,
            true
        );

        //find next path component
        const char *next_component = strchr(dir, '/');
        size_t component_len = next_component ? (next_component - dir) : strlen(dir);
        if (component_len == 0) {
            return curdir;
        }

        uint8_t *ptr = dir_buffer.get();
        uint8_t *end = dir_buffer.get() + dir_size;
        while (ptr < end) {
            if (ptr[0] == 0) {
                ptr++;
                continue;
            }

            auto entry = reinterpret_cast<ISO9660::Entry*>(ptr);
            const char *name = entry->getName();
            uint8_t namelen = entry->name_length;

            //skip "." and ".."
            if (namelen == 1 && (name[0] == '\0' || name[0] == '\1')) {
                ptr += entry->length;
                continue;
            }

            //check if current entry matches the path component
            if (strncmp(dir, name, namelen) == 0 && namelen == component_len) {
                //if no more components, return directory
                if (!next_component || !next_component[1]) {
                    return entry;
                }

                //recurse if it's a directory
                if (entry->flag & ISO9660::FLAG_DIRECTORY) {
                    return readDir(entry, next_component + 1);
                }
            }

            ptr += entry->length;
        }

        return nullptr; //directory not found
    }


    PSXFileSystem::PSXFileSystem(void) {
        //pvd sector
        g_CDInstance.get()->startRead(16, &pvd, sizeof(pvd) / ENGINE::PSX::SECTOR_SIZE, true, true); 
        //assert(pvd.magic == "CD001"_c); //todo _c operator
        printf("build from %s\n", pvd.creation_date);

        rootdir = reinterpret_cast<const ISO9660::Entry*>(&pvd.rootdir);
        auto dir = readDir(rootdir, "DIR1/DIR2");
        // Get the directory name
        printf("Directory name: %s\0\n", dir->getName());
    }
    

} 