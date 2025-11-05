#pragma once

#include "../common.hpp"
#include <stdio.h>

namespace ENGINE::PSX {

    constexpr int SECTOR_SIZE = 2048;

    class CDRom {
    public:
        void issueCMD(uint8_t cmd, const uint8_t *arg, int argLength);
        bool startRead(uint32_t lba, void *ptr, int numSectors, bool doubleSpeed, bool wait);

        void irqDataReady(void);
        void irqComplete(void);
        void irqAcknowledge(void);
        void irqDataEnd(void);
        void irqError(void);
        
        uint8_t response[16];
        uint8_t responselen;

        static CDRom &instance();

    protected:
        volatile bool waitingForDataReady, waitingForComplete, waitingForAcknowledge, waitingForDataEnd, waitingForError;
        bool dataReady;
        void *readPtr;
        int readSectorSize;
        int readNumSectors;
        uint8_t status;
        bool erroroccured;
        
        void waitDataReady(void) {
            while(waitingForDataReady && waitingForError)
                __asm__ volatile("");
        }

        void waitComplete(void){
            while(waitingForComplete)
                __asm__ volatile("");
        }

        void waitAcknowledge(void){
            while(waitingForAcknowledge && waitingForError)
                __asm__ volatile("");

        }

        CDRom() {};
    };

    extern TEMPLATES::ServiceLocator<CDRom> g_CDInstance;
}