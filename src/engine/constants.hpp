#pragma once
#include <stdint.h>

namespace ENGINE::CONST {
    constexpr uint16_t SCREEN_WIDTH  = 320;
    constexpr uint16_t SCREEN_HEIGHT = 240;

    //only useful on ps1
    constexpr uint8_t DMA_MAX_CHUNK_SIZE   = 16;
    constexpr uint16_t CHAIN_BUFFER_SIZE   = 4104;
    constexpr uint16_t ORDERING_TABLE_SIZE = 1024;
    constexpr uint16_t SECTOR_SIZE = 2048;
    // The GTE uses a 20.12 fixed-point format for most values. What this means is
    // that fractional values will be stored as integers by multiplying them by a
    // fixed unit, in this case 4096 or 1 << 12 (hence making the fractional part 12
    // bits long). We'll define this unit value to make their handling easier.
    constexpr uint16_t GTE_ONE = (1 << 12);


    constexpr uint8_t ASSET_MAX = 32;
} //namespace ENGINE::CONST