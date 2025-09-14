#pragma once

#include <stdint.h>
#ifdef PLATFORM_PSX
#include "ps1/gte.h"

// The GTE uses a 20.12 fixed-point format for most values. What this means is
// that fractional values will be stored as integers by multiplying them by a
// fixed unit, in this case 4096 or 1 << 12 (hence making the fractional part 12
// bits long). We'll define this unit value to make their handling easier.
namespace GTE {

constexpr uint16_t ONE = (1 << 12);

void setupGTE(void);
void multiplyCurrentMatrixByVectors(GTEMatrix *output);
void rotateCurrentMatrix(int yaw, int pitch, int roll);
};
#else 

typedef struct __attribute__((aligned(4))) {
	int16_t x, y;
	int16_t z, _padding;
} GTEVector16;

#endif