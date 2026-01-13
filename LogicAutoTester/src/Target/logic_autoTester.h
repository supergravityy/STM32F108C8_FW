/*
 * logic_autoTester.h
 *
 *  Created on: Jan 9, 2026
 *      Author: Smersh
 */

#pragma once

#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*---- TESTER user defines ----*/
extern UART_HandleTypeDef huart2;

#define	LAT_UART_HANDLER		(&huart2)
#define LAT_DISABLE_ISR()		__disable_irq()
#define LAT_ENABLE_ISR()		__enable_irq()

#define LAT_RX_LINE_LENGTH    	(32)
#define LAT_TX_LINE_LENGTH		(64)
#define LAT_1ST_TIMEOUT_MS		(3000)
#define LAT_RX_TIMEOUT_MS		(500)
#define LAT_TX_TIMEOUT_MS		(500)

#define LAT_PC_TO_MCU_RDY		("RDY")
#define LAT_PC_TO_MCU_GO		("GO")
#define LAT_PC_TO_MCU_ACK		("ACK")
#define LAT_MCU_TO_PC_END		("END")
#define LAT_MCU_TO_PC_ACK		("ACK")

/*---- TESTER type defines ----*/

#pragma pack(push, 1)

typedef enum errcode
{
	LAT_NO_ERROR = 0,
	LAT_HANDSHAKE_BGN_WRONG_RX,
	LAT_HANDSHAKE_BGN_TIMEOUT_RX,
	LAT_HANDSHAKE_BGN_LENGTH_TX,
	LAT_HANDSHAKE_BGN_TIMEOUT_TX,
	LAT_HANDSHAKE_END_WRONG_RX,
	LAT_HANDSHAKE_END_TIMEOUT_RX,
	LAT_HANDSHAKE_END_LENGTH_TX,
	LAT_HANDSHAKE_END_TIMEOUT_TX,
	LAT_TRANSMIT_TIMEOUT,
	LAT_TRANSMIT_WRONG_LENGTH,
	LAT_RECEIVE_TIMEOUT,
	LAT_RECEIVE_WRONG_STRING,
	LAT_LOGICTEST_FAILED,
	LAT_LOGICTEST_NONE
}typLAT_errCode;

#pragma pack(pop)

/*---- user api ----*/

typLAT_errCode LAT_process_superLoop(void);
bool LAT_send_data(const char *msg, uint32_t timeout_ms);
bool LAT_wait_data(const char *expectedData, uint32_t timeout_ms);
