/**
 * @file ieee754.h  IEEE754 binary to float converter API
 *
 * Copyright (C) 2022 Christian Spielberger
 */
#ifndef IEEE754_H
#define IEEE754_H

#include <stdint.h>
float ieee754_to_float(uint32_t binary);

#endif
