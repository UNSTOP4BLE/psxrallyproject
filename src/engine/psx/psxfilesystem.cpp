#include "../filesystem.hpp"
#include <assert.h>
#include <stdio.h>
#include <string.h>

namespace ENGINE::PSX {
   
    const ISO9660::Entry* getEntry(const ISO9660::Entry *curdir, const char *path) {
        if (!path || !path[0]) {
            return curdir;
        }

        //allocate buffer for current entry
        uint32_t ent_lba = curdir->lba.le;
        size_t ent_size = curdir->datalength.le;
        size_t ent_num_sectors = (ent_size + ENGINE::PSX::SECTOR_SIZE - 1) / ENGINE::PSX::SECTOR_SIZE;

        ENGINE::TEMPLATES::UniquePtr<uint8_t[]> ent_buffer(new uint8_t[ent_num_sectors * ENGINE::PSX::SECTOR_SIZE]);
        ENGINE::PSX::g_CDInstance.get()->startRead(
            ent_lba,
            ent_buffer.get(),
            ent_num_sectors,
            true,
            true
        );

        //find next path component
        const char *next_component = strchr(path, '/');
        size_t component_len = next_component ? (next_component - path) : strlen(path);
        if (component_len == 0) {
            return curdir;
        }

        uint8_t *ptr = ent_buffer.get();
        uint8_t *end = ent_buffer.get() + ent_size;
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

            //check if current entry matches the entry
            if (strncmp(path, name, namelen) == 0 && namelen == component_len) {
                //if no more components, return entry
                if (!next_component || !next_component[1]) {
                    return entry;
                }

                //recurse if it's a directory
                if (entry->flag & ISO9660::FLAG_DIRECTORY) {
                    return getEntry(entry, next_component + 1);
                }
            }

            ptr += entry->length;
        }

        return nullptr; //entry not found
    }

    //file
    bool PSXFile::open(const char *path) {
        auto file = getEntry(reinterpret_cast<PSXFileSystem *>(g_fileSystemInstance.get())->rootdir, path);
        
        if (file == nullptr)
            return false;

        _startLBA = file->lba.le;
        size = file->datalength.le;
        return true;
    }

    uint32_t PSXFile::read(void *output, uint32_t length) {
        auto ptr    = uintptr_t(output);
        auto offset = uint32_t(_offset);

        // Do not read any data past the end of the file.
        length = ENGINE::COMMON::min(length, size_t(size) - offset);

        for (auto remaining = length; remaining > 0;) {
            auto sectorOffset = offset / SECTOR_SIZE;
            auto ptrOffset    = offset % SECTOR_SIZE;

            auto   lba    = _startLBA + sectorOffset;
            auto   buffer = reinterpret_cast<void *>(ptr);
            size_t readLength;

            if (
                !ptrOffset &&
                (remaining >= SECTOR_SIZE) &&
                ENGINE::COMMON::isBufferAligned(buffer)
            ) {
                // If the read offset is on a sector boundary, at least one sector's
                // worth of data needs to be read and the pointer satisfies any DMA
                // alignment requirements, read as many full sectors as possible
                // directly into the output buffer.
                auto numSectors = remaining / SECTOR_SIZE;
                auto remainder  = remaining % SECTOR_SIZE;
                readLength      = remaining - remainder;

                if (!g_CDInstance.get()->startRead(lba, buffer, numSectors, true, true))
                    return 0;
            } else {
                // In all other cases, read one sector at a time into the sector
                // buffer and copy the requested data over.
                readLength =
                    ENGINE::COMMON::min(remaining, SECTOR_SIZE - ptrOffset);

                if (!loadSector(lba))
                    return 0;

                __builtin_memcpy(buffer, &_sectorBuffer[ptrOffset], readLength);
            }

            ptr       += readLength;
            offset    += readLength;
            remaining -= readLength;
        }

        _offset += length;
        return length;
    }

    uint64_t PSXFile::seek(uint64_t offset) {
        _offset = ENGINE::COMMON::min(offset, size); 
        
        return _offset;
    }

    void PSXFile::close(void) {

    }

    bool PSXFile::loadSector(uint32_t lba) {
        if (lba == _bufferedLBA)
            return true;
        if (!g_CDInstance.get()->startRead(lba, _sectorBuffer, 1, true, true))
            return false;
    
        _bufferedLBA = lba;
        return true;
    }

    PSXFileSystem::PSXFileSystem(void) {
        //pvd sector
        g_CDInstance.get()->startRead(16, &pvd, sizeof(pvd) / ENGINE::PSX::SECTOR_SIZE, true, true); 
        //assert(pvd.magic == "CD001"_c); //todo _c operator
        printf("build from %s\n", pvd.creation_date);

        rootdir = reinterpret_cast<const ISO9660::Entry*>(&pvd.rootdir);
//        auto dir = getEntry(rootdir, "DIR1/DIR2");
        // Get the directory name
  //      printf("Directory name: %s\0\n", dir->getName());
    }
    

} 