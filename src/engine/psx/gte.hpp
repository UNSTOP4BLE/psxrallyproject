#pragma once

#include <stdint.h>
#include <ps1/gte.h>

namespace ENGINE::PSX {

    void setupGTE(const int scrw, const int scrh, const int otlen);
    void multiplyCurrentMatrixByVectors(GTEMatrix *output);
    void rotateCurrentMatrix(int yaw, int pitch, int roll);

} //namespace ENGINE::PSX 