/*
* This is a fast lookup-table-less implementation of fixed-point sine and
* cosine, based on the isin_S4 implementation from:
*     https://www.coranac.com/2009/07/sines
*/

#include "trig.h"

namespace TRIG {
#define A (1 << 12)
#define B 19900
#define	C  3516

int isin(int x) {
	int _c = x << (30 - ISIN_SHIFT);
	x     -= 1 << ISIN_SHIFT;

	x <<= 31 - ISIN_SHIFT;
	x >>= 31 - ISIN_SHIFT;
	x  *= x;
	x >>= 2 * ISIN_SHIFT - 14;

	int _y = B - (x *  C >> 14);
	_y     = A - (x * _y >> 16);

	return (_c >= 0) ? _y : (-_y);
}

int isin2(int x) {
	int _c = x << (30 - ISIN2_SHIFT);
	x    -= 1 << ISIN2_SHIFT;

	x <<= 31 - ISIN2_SHIFT;
	x >>= 31 - ISIN2_SHIFT;
	x  *= x;
	x >>= 2 * ISIN2_SHIFT - 14;

	int _y = B - (x *  C >> 14);
	_y     = A - (x * _y >> 16);

	return (_c >= 0) ? _y : (-_y);
}
}