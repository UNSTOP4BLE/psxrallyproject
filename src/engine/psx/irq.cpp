#include "irq.hpp"

#include "ps1/system.h"
#include "ps1/cop0.h"
#include "ps1/cdrom.h"

#include "../timer.hpp"
#include "../renderer.hpp"

namespace ENGINE::PSX {

void handleT2Interrupt(void) {
	__atomic_signal_fence(__ATOMIC_ACQUIRE);
    reinterpret_cast<PSXTimer *>(g_timerInstance.get())->t2irqcount ++;
	__atomic_signal_fence(__ATOMIC_RELEASE);
}

void handleCDROMInterrupt(void) {
    CDROM_ADDRESS = 1;

    uint8_t irqType = CDROM_HINTSTS & (0
        | CDROM_HINT_INT0
        | CDROM_HINT_INT1
        | CDROM_HINT_INT2);

    // If a new sector is available, request a sector buffer read.
    if (irqType == CDROM_IRQ_DATA_READY) {
        CDROM_ADDRESS = 0;
        CDROM_HCHPCTL = 0;
        CDROM_HCHPCTL = CDROM_HCHPCTL_BFRD;
    }

    CDROM_ADDRESS = 1;
    CDROM_HCLRCTL = 0 // Acknowledge all IRQs
        | CDROM_HCLRCTL_CLRINT0
        | CDROM_HCLRCTL_CLRINT1
        | CDROM_HCLRCTL_CLRINT2;
    CDROM_HCLRCTL = CDROM_HCLRCTL_CLRPRM; // Clear parameter buffer
    delayMicroseconds(3);

    //cdromRespLength = 0;

    //while (CDROM_HSTS & CDROM_HSTS_RSLRRDY)
   //     cdromResponse[cdromRespLength++] = CDROM_RESULT;

    switch (irqType) {
        case CDROM_IRQ_DATA_READY:
            //cdromDataReady();
            break;
        case CDROM_IRQ_COMPLETE:
      //      cdromComplete();
            break;
        case CDROM_IRQ_ACKNOWLEDGE:
            //cdromAcknowledge();
            break;
        case CDROM_IRQ_DATA_END:
         //   cdromDataEnd();
            break;
        case CDROM_IRQ_ERROR:
          //  cdromError();
            break;
    }
}

// This is the first step to handling the IRQ.
// It will acknowledge the interrupt on the COP0 side, and call the relevant handler for the device.
static void handleInterrupts(void *arg0, void *arg1){
    if (acknowledgeInterrupt(IRQ_TIMER2)){
        handleT2Interrupt();
    }
//    if (acknowledgeInterrupt(IRQ_CDROM)){
  //      handleCDROMInterrupt();
    //}
    if (acknowledgeInterrupt(IRQ_VSYNC)){
        reinterpret_cast<PSXRenderer *>(g_rendererInstance.get())->handleVSyncInterrupt();
    }
}

void initIRQ(void){
    installExceptionHandler();
    // This is the function that is called when an interrupt is raised.
    // You can also pass an argument to this handler.
    setInterruptHandler(handleInterrupts, nullptr, nullptr);

    // The IRQ mask specifies which interrupt sources are actually allowed to raise an interrupt.
    IRQ_MASK = 0 | 
            (1 << IRQ_TIMER2) | 
            (1 << IRQ_CDROM) |
            (1 << IRQ_VSYNC);
    cop0_enableInterrupts();
}


}
