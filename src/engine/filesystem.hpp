#pragma once 

#include "templates.hpp"

#ifdef PLATFORM_PSX
#include "psx/cd.hpp"
#else 
#include <stdio.h>
#include <stdint.h>
#endif

namespace ENGINE { 

    class File {
    public:
        virtual uint32_t read(void *output, uint32_t length) { return 0; }
        virtual uint64_t seek(uint64_t offset) { return 0; }
        virtual uint64_t tell(void) { return 0; }
        uint64_t getSize(void) const { return _size; }
    protected:
        uint64_t _size;
    };

    class FileSystem {
    public:
        static FileSystem &instance();
        virtual File *findFile(const char *path) { return nullptr; }
    protected:
        FileSystem() {}
        virtual ~FileSystem() = default;
    };
 
    extern TEMPLATES::ServiceLocator<FileSystem> g_fileSystemInstance;

    //psx
#ifdef PLATFORM_PSX
    namespace PSX {
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

            struct [[gnu::packed]] Entry {
                uint8_t  length;                   
                uint8_t  ext_attr_length;         
                ISOUint32 lba;  
                ISOUint32 datalength;
                ISODate date;        //Recording date and time
                uint8_t  flag;           
                uint8_t  interleave_length;        
                uint8_t  interleave_gap_size;    
                ISOUint16 volumeid;      
                uint8_t  name_length;  

                inline const char *getName(void) const {
                    return reinterpret_cast<const char *>(&name_length + 1);
                }
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
        
        class PSXFile : public File {
            friend class PSXFileSystem;
        public:
            uint32_t read(void *output, uint32_t length);
            uint64_t seek(uint64_t offset);
            uint64_t tell(void) { return _offset; }
        protected:
        	uint32_t _startLBA;
            uint32_t _bufferedLBA;
            uint8_t  _sectorBuffer[SECTOR_SIZE];    
            uint64_t _offset;
            bool loadSector(uint32_t lba);
        };

        class PSXFileSystem : public FileSystem {
        public:
            PSXFileSystem(void);	
            File *findFile(const char *path); 
        private:
            const ISO9660::Entry* rootdir;
            ISO9660::PVD pvd;
        };
    } 
#else 
    namespace GENERIC {

        class GenericFile : public File {
            friend class GenericFileSystem;
        public:
            uint32_t read(void *output, uint32_t length);
            uint64_t seek(uint64_t offset);
            uint64_t tell(void) { return ftell(_handle); }
        protected:
            FILE* _handle;
            ~GenericFile(void) {if (_handle) fclose(_handle);}
        };

        class GenericFileSystem : public FileSystem {
        public:
            GenericFileSystem(void);	
            File *findFile(const char *path); 
        };
    }
#endif
}