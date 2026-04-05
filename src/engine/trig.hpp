#pragma once

//todo clean up
namespace ENGINE::TRIG {
	constexpr uint8_t ISIN_SHIFT  = 10;
	constexpr uint8_t ISIN2_SHIFT = 15;
	constexpr uint32_t ISIN_PI = (1 << (ISIN_SHIFT  + 1));
	constexpr uint32_t ISIN2_PI = (1 << (ISIN2_SHIFT + 1));

	int isin(int x);
	int isin2(int x);

	static inline int icos(int x) {
		return isin(x + (1 << ISIN_SHIFT));
	}
	static inline int icos2(int x) {
		return isin2(x + (1 << ISIN2_SHIFT));
	}
} //namespace TRIG