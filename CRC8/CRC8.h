#pragma once

#include <stdint.h>
#include <stdbool.h>

/*--------------------------------*/
// include User Libs START
/*--------------------------------*/

/*--------------------------------*/
// include User Libs END
/*--------------------------------*/

uint8_t CRC8_calcCRC(const uint8_t* pcktData, uint8_t pcktLength);
bool CRC8_chkIntegrity(const uint8_t* pcktData, uint8_t pcktLength);
void CRC8_fillPckt(void* packet);