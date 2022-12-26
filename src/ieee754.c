/**
 * @file ieee754.c  IEEE754 binary to float converter
 *
 * Copyright (C) 2022 Christian Spielberger
 */

#include <stdint.h>

typedef union {

	float f;
	struct
	{
		/**
		 * The float and the struct are sharing the same 32 bit
		 * memory. The order is taken from the LSB to the MSB.
		 */

		uint32_t mantissa : 23;
		uint32_t exponent : 8;
		uint32_t sign : 1;

	} d;
} converter;


float ieee754_to_float(uint32_t binary)
{
	converter conv;

	/* mantissa part (23 bits) */
	uint32_t f = binary & 0x007fffff;
	conv.d.mantissa = f;

	/* exponent part (8 bits) */
	f = (binary & 0x7fffffff) >> 23;
	conv.d.exponent = f;

	/* sign bit */
	conv.d.sign = binary & 0x80000000;

	return conv.f;
}
