#include "../filesystem.hpp"
#include <assert.h>
#include <stdio.h>

namespace ENGINE::PSX {
    namespace ISO9660 {
        template<typename T> struct [[gnu::packed]] ISOInt {
            T le, be;
        };

        struct [[gnu::packed]] ISODate {
            uint8_t year, month, day, hour, minute, second, timezone;
        };

        enum ISORecordFlag : uint8_t {
            FLAG_EXISTS       = 1 << 0,
            FLAG_DIRECTORY    = 1 << 1,
            FLAG_ASSOCIATED   = 1 << 2,
            FLAG_EXT_ATTR     = 1 << 3,
            FLAG_PROTECTION   = 1 << 4,
            FLAG_MULTI_EXTENT = 1 << 7
        };

        using ISOUint16 = ISOInt<uint16_t>;
        using ISOUint32 = ISOInt<uint32_t>;
        using ISOChar  = char;

        struct [[gnu::packed]] Directory {
            uint8_t  length;                   
            uint8_t  ext_attr_length;         
            ISOUint32 lba;  
            ISOUint32 datalength;
            ISODate date;        //Recording date and time
            uint8_t  fileflags;           
            uint8_t  interleave_length;        
            uint8_t  interleave_gap_size;    
            ISOUint16 volumeid;      
            uint8_t  filename_length;  
        };

        struct [[gnu::packed]] PVD {    
            uint8_t type; 
            uint8_t magic[5]; //always CD001
            uint8_t version;  
            uint8_t _unused1;
            uint8_t sysid[32];
            uint8_t volumeid[32];
            uint8_t _unused2[8];

            ISOUint32 volumesize; 

            uint8_t  _unused3[32];       

            ISOUint16 numvolumes;    

            ISOUint16 volume;

            ISOUint16 blocksize;   

            ISOUint32 pathtablesize;
            
            uint32_t pathtable_le;         
            uint32_t opt_pathtable_le;   

            uint32_t pathtable_be;         
            uint32_t opt_pathtable_be;    

            uint8_t  rootdir[34];       

            uint8_t  volumesetid[128];
            uint8_t  publisherid[128];
            uint8_t  datapreparerid[128];
            uint8_t  appid[128];
            uint8_t  copyright_fileid[37];
            uint8_t  abstract_fileid[37];
            uint8_t  bibliographic_fileid[37];

            uint8_t  creation_date[17];        // YYYYMMDDHHMMSSCC
            uint8_t  modification_date[17];
            uint8_t  expiration_date[17];
            uint8_t  effective_date[17];

            uint8_t  filestructver;  
            uint8_t  _unused4;          

            uint8_t  application_data[512]; 
            uint8_t  _unused5[653];            
        };
        static_assert(sizeof(PVD) == 2048, "PVD must be exactly 2048 bytes");
    }

    PSXFileSystem::PSXFileSystem(void) {
        ISO9660::PVD pvd;
        //pvd sector
        g_CDInstance.get()->startRead(16, &pvd, sizeof(pvd) / ENGINE::PSX::SECTOR_SIZE, true, true); 
        //assert(pvd.magic == "CD001"_c); //todo _c operator
        printf("build from %s\n", pvd.creation_date);

        const ISO9660::Directory* rootdir = reinterpret_cast<const ISO9660::Directory*>(&pvd.rootdir);
    }
    
    const ISO9660::Directory* PSXFileSystem::readDir(const ISO9660::Directory *curdir, const char *dir) {
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


} 