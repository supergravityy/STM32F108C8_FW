/*
 * logic_test.c
 *
 *  Created on: Jan 11, 2026
 *      Author: Smersh
 */

#include "logic_test.h"
#include "main.h"
#include "../tester/logic_autoTester.h"
#include "ipol.h"
#include <stdio.h>

#define LOGIC_TEST_MAPSIZE		(20)
#define LOGIC_TEST_RSLT_SIZE	(64)

uint8_t logicTest_resultArr [LOGIC_TEST_RSLT_SIZE];

uint16_t logicTest_u16u16Val;
uint32_t logicTest_u16u32Val;
int16_t logicTest_s16s16Val;
uint16_t logicTest_s16u16Val;
int16_t logicTest_u32s16Val;

/*---- define sampleX ----*/

const uint16_t sampleX_u16[LOGIC_TEST_MAPSIZE] =
{0, 10, 25, 40, 60, 85, 120, 160, 210, 270,
	340, 420, 510, 610, 720, 840, 970, 1110, 1260, 1420};

const int16_t sampleX_s16[LOGIC_TEST_MAPSIZE] =
{0, 10, 25, 40, 60, 85, 120, 160, 210, 270,
	340, 420, 510, 610, 720, 840, 970, 1110, 1260, 1420};

const uint32_t sampleX_u32[LOGIC_TEST_MAPSIZE] =
{0, 10, 25, 40, 60, 85, 120, 160, 210, 270,
	340, 420, 510, 610, 720, 840, 970, 1110, 1260, 1420};

const int32_t sampleX_s32[LOGIC_TEST_MAPSIZE] =
{0, 10, 25, 40, 60, 85, 120, 160, 210, 270,
	340, 420, 510, 610, 720, 840, 970, 1110, 1260, 1420};

/*---- define sampleY ----*/

const uint16_t sampleY_u16[LOGIC_TEST_MAPSIZE] =
{ 0, 15, 40, 70, 110, 165, 240, 330, 450, 600,
		 780, 1000, 1260, 1560, 1900, 2280, 2700, 3160, 3660, 4200 };

const int16_t sampleY_s16[LOGIC_TEST_MAPSIZE] =
{ 0, 15, 40, 70, 110, 165, 240, 330, 450, 600,
		 780, 1000, 1260, 1560, 1900, 2280, 2700, 3160, 3660, 4200 };

const uint32_t sampleY_u32[LOGIC_TEST_MAPSIZE] =
{ 0, 15, 40, 70, 110, 165, 240, 330, 450, 600,
		 780, 1000, 1260, 1560, 1900, 2280, 2700, 3160, 3660, 4200 };

const int32_t sampleY_s32[LOGIC_TEST_MAPSIZE] =
{ 0, 15, 40, 70, 110, 165, 240, 330, 450, 600,
		 780, 1000, 1260, 1560, 1900, 2280, 2700, 3160, 3660, 4200 };

/*---- packaging calculated Result and Send ----*/

bool logicTest_cnvrtTostr_andSend(uint16_t tcx1, uint32_t tcx2, int16_t tcx3, uint16_t tcx4, int16_t tcx5)
{
	// number -> str
	// ** CAUTION : logic auto tester by UART use data by string **

	snprintf((char *)logicTest_resultArr, LOGIC_TEST_RSLT_SIZE, "%u,%lu,%d,%u,%d",
			tcx1, (unsigned long) tcx2, tcx3, tcx4, tcx5);

	return LAT_send_data((const char*)logicTest_resultArr, LAT_TX_TIMEOUT_MS);
}

/*---- TC0X ----*/
bool logicTest_TC0X(void)
{
	// TC01
	logicTest_u16u16Val = ipol_u16u16(sampleX_u16, sampleY_u16, LOGIC_TEST_MAPSIZE, 1500);
	// TC02
	logicTest_u16u32Val = ipol_u16u32(sampleX_u16, sampleY_u32, LOGIC_TEST_MAPSIZE, 2000);
	// TC03
	logicTest_s16s16Val = ipol_s16s16(sampleX_s16, sampleY_s16, LOGIC_TEST_MAPSIZE, 1600);
	// TC04
	logicTest_s16u16Val = ipol_s16u16(sampleX_s16, sampleY_u16, LOGIC_TEST_MAPSIZE, 1500);
	// TC05
	logicTest_u32s16Val = ipol_u32s16(sampleX_u32, sampleY_s16, LOGIC_TEST_MAPSIZE, 50000);

	return logicTest_cnvrtTostr_andSend(logicTest_u16u16Val,
			logicTest_u16u32Val, logicTest_s16s16Val, logicTest_s16u16Val,
			logicTest_u32s16Val);
}
/*---- TC1X ----*/
bool logicTest_TC1X(void)
{
	// TC11
	logicTest_u16u16Val = ipol_u16u16(sampleX_u16, sampleY_u16, LOGIC_TEST_MAPSIZE, 0);
	// TC12
	logicTest_u16u32Val = ipol_u16u32(sampleX_u16, sampleY_u32, LOGIC_TEST_MAPSIZE, 0);
	// TC13
	logicTest_s16s16Val = ipol_s16s16(sampleX_s16, sampleY_s16, LOGIC_TEST_MAPSIZE, -100);
	// TC14
	logicTest_s16u16Val = ipol_s16u16(sampleX_s16, sampleY_u16, LOGIC_TEST_MAPSIZE, -50);
	// TC15
	logicTest_u32s16Val = ipol_u32s16(sampleX_u32, sampleY_s16, LOGIC_TEST_MAPSIZE, 0);

	return logicTest_cnvrtTostr_andSend(logicTest_u16u16Val,
			logicTest_u16u32Val, logicTest_s16s16Val, logicTest_s16u16Val,
			logicTest_u32s16Val);
}
/*---- TC2X ----*/
bool logicTest_TC2X(void)
{
	// TC21
	logicTest_u16u16Val = ipol_u16u16(sampleX_u16, sampleY_u16, LOGIC_TEST_MAPSIZE, 510);
	// TC22
	logicTest_u16u32Val = ipol_u16u32(sampleX_u16, sampleY_u32, LOGIC_TEST_MAPSIZE, 510);
	// TC23
	logicTest_s16s16Val = ipol_s16s16(sampleX_s16, sampleY_s16, LOGIC_TEST_MAPSIZE, 510);
	// TC24
	logicTest_s16u16Val = ipol_s16u16(sampleX_s16, sampleY_u16, LOGIC_TEST_MAPSIZE, 510);
	// TC25
	logicTest_u32s16Val = ipol_u32s16(sampleX_u32, sampleY_s16, LOGIC_TEST_MAPSIZE, 510);

	return logicTest_cnvrtTostr_andSend(logicTest_u16u16Val,
			logicTest_u16u32Val, logicTest_s16s16Val, logicTest_s16u16Val,
			logicTest_u32s16Val);
}
/*---- TC3X ----*/
bool logicTest_TC3X(void)
{
	// TC31
	logicTest_u16u16Val = ipol_u16u16(sampleX_u16, sampleY_u16, LOGIC_TEST_MAPSIZE, 300);
	// TC32
	logicTest_u16u32Val = ipol_u16u32(sampleX_u16, sampleY_u32, LOGIC_TEST_MAPSIZE, 300);
	// TC33
	logicTest_s16s16Val = ipol_s16s16(sampleX_s16, sampleY_s16, LOGIC_TEST_MAPSIZE, 300);
	// TC34
	logicTest_s16u16Val = ipol_s16u16(sampleX_s16, sampleY_u16, LOGIC_TEST_MAPSIZE, 300);
	// TC35
	logicTest_u32s16Val = ipol_u32s16(sampleX_u32, sampleY_s16, LOGIC_TEST_MAPSIZE, 300);

	return logicTest_cnvrtTostr_andSend( logicTest_u16u16Val,
			logicTest_u16u32Val, logicTest_s16s16Val, logicTest_s16u16Val,
			logicTest_u32s16Val);
}
/*---- TC4X ----*/
bool logicTest_TC4X(void)
{
	// TC41
	logicTest_u16u16Val = ipol_u16u16(sampleX_u16, sampleY_u16, LOGIC_TEST_MAPSIZE, 115);
	// TC42
	logicTest_u16u32Val = ipol_u16u32(sampleX_u16, sampleY_u32, LOGIC_TEST_MAPSIZE, 115);
	// TC43
	logicTest_s16s16Val = ipol_s16s16(sampleX_s16, sampleY_s16, LOGIC_TEST_MAPSIZE, 115);
	// TC44
	logicTest_s16u16Val = ipol_s16u16(sampleX_s16, sampleY_u16, LOGIC_TEST_MAPSIZE, 115);
	// TC45
	logicTest_u32s16Val = ipol_u32s16(sampleX_u32, sampleY_s16, LOGIC_TEST_MAPSIZE, 115);

	return logicTest_cnvrtTostr_andSend(logicTest_u16u16Val,
			logicTest_u16u32Val, logicTest_s16s16Val, logicTest_s16u16Val,
			logicTest_u32s16Val);
}

/*---- reDefined userLogic for test ----*/

bool LAT_logic_test(void)
{
	if (logicTest_TC0X() == false)
		return false;
	else
		HAL_Delay(10); // wait a few ms for processing pc's buffer to empty

	if (logicTest_TC1X() == false)
		return false;
	else
		HAL_Delay(10);

	if (logicTest_TC2X() == false)
		return false;
	else
		HAL_Delay(10);

	if (logicTest_TC3X() == false)
		return false;
	else
		HAL_Delay(10);

	if(logicTest_TC4X() == false)
		return false;
	else
		return true;
}
