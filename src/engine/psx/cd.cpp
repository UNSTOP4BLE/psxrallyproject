#include "cd.hpp"
#include <ps1/registers.h>
#include <ps1/system.h>
#include <ps1/cdrom.h>
#include <assert.h>

namespace ENGINE::PSX {
COMMON::ServiceLocator<CDRom> g_CDInstance;

CDRom &CDRom::instance(void) {
    BIU_DEV5_CTRL = 0x00020943; // Configure bus
    DMA_DPCR |= DMA_DPCR_CH_ENABLE(DMA_CDROM); // Enable DMA

    CDROM_ADDRESS = 1;
    CDROM_HCLRCTL = 0 // Acknowledge all IRQs
        | CDROM_HCLRCTL_CLRINT0
        | CDROM_HCLRCTL_CLRINT1
        | CDROM_HCLRCTL_CLRINT2;
    CDROM_HINTMSK_W = 0 // Enable all IRQs
        | CDROM_HCLRCTL_CLRINT0
        | CDROM_HCLRCTL_CLRINT1
        | CDROM_HCLRCTL_CLRINT2;

    CDROM_ADDRESS = 0;
    CDROM_HCHPCTL = 0; // Clear pending requests

    CDROM_ADDRESS = 2;
    CDROM_ATV0 = 128; // Send left audio channel to SPU left channel
    CDROM_ATV1 = 0;

    CDROM_ADDRESS = 3;
    CDROM_ATV2 = 128; // Send right audio channel to SPU right channel
    CDROM_ATV3 = 0;
    CDROM_ADPCTL = CDROM_ADPCTL_CHNGATV;

    static CDRom *instance = new CDRom();
    //reset vars
    instance->waitingForDataReady = false;
    instance->waitingForComplete = false;
    instance->waitingForAcknowledge = false;
    instance->waitingForDataEnd = false;
    instance->waitingForError = false; 

    instance->dataReady = false; 
    
    instance->readPtr = nullptr;
    instance->readSectorSize = 0;
    instance->readNumSectors = 0;
    
    return *instance;
};

void CDRom::issueCMD(uint8_t cmd, const uint8_t *arg, int argLength) {
    waitingForDataReady = true;
    waitingForComplete = true;
    waitingForAcknowledge = true;
    waitingForDataEnd = true;
    waitingForError = true;

    dataReady = false;

//    while (CDROM_BUSY)
    //    __asm__ volatile("");
    
    CDROM_ADDRESS = 1;
    CDROM_HCLRCTL = CDROM_HCLRCTL_CLRPRM; // Clear parameter buffer
    delayMicroseconds(3);
  //  while (CDROM_BUSY)
      //  __asm__ volatile("");

    CDROM_ADDRESS = 0;
    for (; argLength > 0; argLength--)
        CDROM_PARAMETER = *(arg++);
    
    CDROM_COMMAND = cmd;
}

void CDRom::startRead(uint32_t lba, void *ptr, int numSectors, int sectorSize, bool doubleSpeed, bool wait) {
    readPtr = ptr;
    readNumSectors = numSectors;
    readSectorSize = sectorSize;

    uint8_t mode = 0;
    CDROMMSF msf;

    if (sectorSize == 2340)
        mode |= CDROM_MODE_SIZE_2340;
    if (doubleSpeed)
        mode |= CDROM_MODE_SPEED_2X;

    cdrom_convertLBAToMSF(&msf, lba);
    issueCMD(CDROM_CMD_SETMODE, &mode, sizeof(mode));
    waitAcknowledge();
    issueCMD(CDROM_CMD_SETLOC, reinterpret_cast<const uint8_t *>(&msf), sizeof(msf));
    waitAcknowledge();
    issueCMD(CDROM_CMD_READ_N, nullptr, 0);
    waitAcknowledge();

    assert(waitingForDataReady);

    if (wait)
    {
        while (waitingForDataReady)
        {
            // busy wait
            delayMicroseconds(2);
        }
    }
}

//int1
void CDRom::irqDataReady(void) {

}
//int2
void CDRom::irqComplete(void) {

}

//int3
void CDRom::irqAcknowledge(void) {

}

//int4
void CDRom::irqDataEnd(void) {

}

//int5
void CDRom::irqError(void) {

}

}