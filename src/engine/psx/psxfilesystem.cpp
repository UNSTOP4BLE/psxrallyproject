#include "../filesystem.hpp"
#include <assert.h>

namespace ENGINE::PSX {
    namespace ISO9660 {
        template<typename T> struct [[gnu::packed]] ISOInt {
            T le, be;
        };
        using ISOUint16 = ISOInt<uint16_t>;
        using ISOUint32 = ISOInt<uint32_t>;
        using ISOChar  = char;

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
        ENGINE::PSX::g_CDInstance.get()->startRead(16, &pvd, sizeof(pvd) / ENGINE::PSX::SECTOR_SIZE, true, true); 
        assert(pvd.magic == "CD001"_c); //todo _c operator
        ENGINE::COMMON::hexDump(reinterpret_cast<const void *>(&pvd), sizeof(pvd));
    }

} 