/**
 * @file util.h  Utility API
 *
 * Copyright (C) 2022 Christian Spielberger
 */
#ifndef FUTIL_H
#define FUTIL_H
#include <stdint.h>
#include <stddef.h>

#define ARRAY_SIZE(a) ((sizeof(a))/sizeof((a)[0]))

void bin_u16(uint16_t v);
void ascii_u16(uint16_t v);
uint16_t rand_u16(void);
uint32_t rand_u32(void);
uint64_t tmr_jiffies(void);
#endif
