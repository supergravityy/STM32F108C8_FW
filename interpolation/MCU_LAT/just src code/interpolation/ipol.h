/*
 * ipol.h
 *
 *  Created on: 2025. 12. 25.
 *      Author: sungsoo
 */

#pragma once

#include <stdint.h>

uint16_t ipol_u16u16(const uint16_t* mapX, const uint16_t* mapY, uint16_t mapSize, uint16_t input);
uint32_t ipol_u16u32(const uint16_t* mapX, const uint32_t* mapY, uint16_t mapSize, uint16_t input);
int16_t ipol_s16s16(const int16_t* mapX, const int16_t* mapY, uint16_t mapSize, int16_t input);
int16_t ipol_s16u16(const int16_t* mapX, const uint16_t* mapY, uint16_t mapSize, int16_t input);
int16_t ipol_u32s16(const uint32_t* mapX, const int16_t* mapY, uint16_t mapSize, uint32_t input);