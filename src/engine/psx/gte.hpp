#pragma once

#include <stdint.h>
#include "ps1/gte.h"

// The GTE uses a 20.12 fixed-point format for most values. What this means is
// that fractional values will be stored as integers by multiplying them by a
// fixed unit, in this case 4096 or 1 << 12 (hence making the fractional part 12
// bits long). We'll define this unit value to make their handling easier.
namespace ENGINE::PSX {

constexpr uint16_t ONE = (1 << 12);

void setupGTE(const int scrw, const int scrh, const int otlen);
void multiplyCurrentMatrixByVectors(GTEMatrix *output);
void rotateCurrentMatrix(int yaw, int pitch, int roll);

} 