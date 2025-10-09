#include "common.hpp"
#include <stdio.h>

namespace ENGINE::COMMON {

    void hexDump(const void* data, uint32_t size) {
        auto bytes = reinterpret_cast<const uint8_t*>(data);
        static const char hex[] = "0123456789ABCDEF";

        for (uint32_t i = 0; i < size; i += 16) {
            // Print offset
            uint32_t offset = i;
            for (int shift = 28; shift >= 0; shift -= 4)
                putchar(hex[(offset >> shift) & 0xF]);
            putchar(' ');
            putchar(' ');

            // Hex part
            for (uint32_t j = 0; j < 16; ++j) {
                if (i + j < size) {
                    uint8_t c = bytes[i + j];
                    putchar(hex[c >> 4]);
                    putchar(hex[c & 0xF]);
                    putchar(' ');
                } else {
                    putchar(' ');
                    putchar(' ');
                    putchar(' ');
                }
            }

            putchar(' ');

            // ASCII part
            for (uint32_t j = 0; j < 16 && i + j < size; ++j) {
                uint8_t c = bytes[i + j];
                if (c >= 32 && c <= 126)
                    putchar(c);
                else
                    putchar('.');
            }

            putchar('\n');
        }
    }

} //namespace ENGINE::COMMON