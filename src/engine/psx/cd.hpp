#pragma once

#include "../common.hpp"

namespace ENGINE::PSX {

class CDRom {
public:
    void issueCMD(uint8_t cmd, const uint8_t *arg, int argLength);
    void startRead(uint32_t lba, void *ptr, int numSectors, int sectorSize, bool doubleSpeed, bool wait);

    static CDRom &instance();

protected:
    volatile bool waitingForDataReady, waitingForComplete, waitingForAcknowledge, waitingForDataEnd, waitingForError;
    bool dataReady;
    void *readPtr;
    int readSectorSize;
    int readNumSectors;

    void waitDataReady(void) {
        while(waitingForDataReady && waitingForError){
            __asm__ volatile("");
        }
    }

    void waitComplete(void){
        while(waitingForComplete){
            __asm__ volatile("");
        }
    }

    void waitAcknowledge(void){
        while(waitingForAcknowledge && waitingForError){
            __asm__ volatile("");
        }
    }

    void irqDataReady(void);
    void irqComplete(void);
    void irqAcknowledge(void);
    void irqDataEnd(void);
    void irqError(void);

    CDRom() {};
};

extern COMMON::ServiceLocator<CDRom> g_CDInstance;
}