/*
 * STMicroelectronics half float to float conversion
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include "half_float.h"

uint32_t halfbits_to_floatbits(uint16_t h)
{
	uint16_t h_exp, h_sig;
	uint32_t f_sgn, f_exp, f_sig;

	h_exp = (h & 0x7c00u);
	f_sgn = ((uint32_t)h & 0x8000u) << 16;
	switch (h_exp) {
	case 0x0000u:
		/* 0 or subnormal */
		h_sig = (h & 0x03ffu);
		/* Signed zero */
		if (h_sig == 0)
			return f_sgn;
		/* Subnormal */
		h_sig <<= 1;
		while ((h_sig & 0x0400u) == 0) {
			h_sig <<= 1;
			h_exp++;
		}
		f_exp = ((uint32_t)(127 - 15 - h_exp)) << 23;
		f_sig = ((uint32_t)(h_sig & 0x03ffu)) << 13;
		return f_sgn + f_exp + f_sig;
	case 0x7c00u:
		/* inf or NaN */
		/* All-ones exponent and a copy of the significand */
		return f_sgn + 0x7f800000u + (((uint32_t)(h & 0x03ffu)) << 13);
	default:
		/* normalized */
		/* Just need to adjust the exponent and shift */
		return f_sgn + (((uint32_t)(h & 0x7fffu) + 0x1c000u) << 13);
    }
}

float half_to_float(uint16_t h)
{
	union {
		float ret;
		uint32_t retbits;
	} conv;

	conv.retbits = halfbits_to_floatbits(h);

	return conv.ret;
}

