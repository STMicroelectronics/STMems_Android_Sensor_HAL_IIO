/*
 * STMicroelectronics half float to float conversion
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#ifndef HFLOAT_2_FLOAT_H
#define HFLOAT_2_FLOAT_H

#include <inttypes.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <float.h>
#include <stdlib.h>
#include <cutils/log.h>

uint32_t halfbits_to_floatbits(uint16_t h);
float half_to_float(uint16_t h);

#endif /* HFLOAT_2_FLOAT_H */
