/*
 * eol.c
 *
 *  Created on: Jan 9, 2026
 *      Author: Smersh
 */

/*---- include ----*/

#include "main.h"
#include "logic_autoTester.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*---- define ----*/

#define LAT_STRING_PREFIX	'$'
#define LAT_STRING_SUFFIX	'\n'

/*---- type defines ----*/

#pragma pack(push,1)

typedef struct RxInfos
{
	char rxByte;

	char rxLineBuffer[LAT_RX_LINE_LENGTH];

	char rxCompareBuff[LAT_RX_LINE_LENGTH];

	uint16_t rxBufferIdx;

	uint16_t rxLineCnt;

	bool rxLineRdy;
}typLAT_RxInfos;

typedef struct TxInfos
{
	char txLineBuffer[LAT_TX_LINE_LENGTH];

	uint8_t txLineBuff_size;

	uint16_t txLineCnt;
}typLAT_TxInfos;

#pragma pack(pop)

/*---- global variable ----*/

static volatile typLAT_RxInfos LAT_rxInfo;
static volatile typLAT_TxInfos LAT_txInfo;
static typLAT_errCode LAT_errCode;

/*---- RxISR ----*/

inline static void LAT_receiveByte(void)
{
	// 1. communication begin
	if(LAT_rxInfo.rxByte == LAT_STRING_PREFIX)
	{
		LAT_rxInfo.rxBufferIdx = 0;
		LAT_rxInfo.rxLineRdy = false;
	}
	// 2. communication end
	else if (LAT_rxInfo.rxByte == LAT_STRING_SUFFIX)
	{
		if(LAT_rxInfo.rxBufferIdx > 0) // must be at least one char
		{
			LAT_rxInfo.rxLineBuffer[LAT_rxInfo.rxBufferIdx] = '\0';

			memset(LAT_rxInfo.rxCompareBuff, 0 , LAT_RX_LINE_LENGTH);
			strcpy(LAT_rxInfo.rxCompareBuff, LAT_rxInfo.rxLineBuffer);
			memset(LAT_rxInfo.rxLineBuffer, 0 , LAT_RX_LINE_LENGTH);

			LAT_rxInfo.rxLineRdy = true;
			LAT_rxInfo.rxLineCnt++;
		}
	}
	// 3. rcv char (ignore '\r')
	else if(LAT_rxInfo.rxByte != '\r' && LAT_rxInfo.rxBufferIdx < (LAT_RX_LINE_LENGTH - 1))
	{
		LAT_rxInfo.rxLineBuffer[LAT_rxInfo.rxBufferIdx++] = LAT_rxInfo.rxByte;
	}
	else
	{
		// do nothing
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == LAT_UART_HANDLER)
	{
		LAT_receiveByte();
	}

	HAL_UART_Receive_IT(LAT_UART_HANDLER, (uint8_t*) &LAT_rxInfo.rxByte, 1);
}

/*---- unit functions----*/

bool LAT_send_data(const char *msg, uint32_t timeout_ms)
{
	char txTempBuf[LAT_TX_LINE_LENGTH + 3]; // get space at the beginning ($) and end (\n)
    bool result;
    uint8_t txTempBuf_length = 0;
    HAL_StatusTypeDef temp = HAL_OK;

    // commu spec : $ + string + \n
    if(LAT_txInfo.txLineBuff_size > LAT_TX_LINE_LENGTH)
    {
    	LAT_errCode = LAT_TRANSMIT_WRONG_LENGTH;
    	result = false;
    }

    else
    {
    	// commu spec : $ + string + \n
    	txTempBuf_length = snprintf(txTempBuf, sizeof(txTempBuf),
    			"%c%s%c", LAT_STRING_PREFIX, msg,LAT_STRING_SUFFIX);

    	temp = HAL_UART_Transmit(LAT_UART_HANDLER, (uint8_t*)txTempBuf,
    			txTempBuf_length, timeout_ms);
    	result = (temp == HAL_OK);

    	if(result == false)
    		LAT_errCode = LAT_TRANSMIT_TIMEOUT;

    	memset(LAT_txInfo.txLineBuffer, 0, LAT_TX_LINE_LENGTH);
    }

    return result;
}

bool LAT_wait_data(const char *expectedData, uint32_t timeout_ms)
{
	uint32_t start = HAL_GetTick();
	bool result;

	while(HAL_GetTick() - start < timeout_ms)
	{
		if(LAT_rxInfo.rxLineRdy == true)
		{
			// ~ critical Section for rxCompareBuff and rxLineRdy member var
			LAT_DISABLE_ISR();

			if(strcmp((const char *)LAT_rxInfo.rxCompareBuff, expectedData) == 0)
			{
				LAT_rxInfo.rxLineRdy = false;

				result = true;
			}
			else
			{
				LAT_errCode = LAT_RECEIVE_WRONG_STRING;

				result = false;
			}

			LAT_ENABLE_ISR();
			break;
		}
	}

	if(HAL_GetTick() - start >= timeout_ms)
	{
		result = false;
		LAT_errCode = LAT_RECEIVE_TIMEOUT;
	}

	return result;
}

void LAT_update_Rxerr_bgn(void)
{
	if (LAT_errCode == LAT_RECEIVE_WRONG_STRING)
	{
		LAT_errCode = LAT_HANDSHAKE_BGN_WRONG_RX;
	}
	else if (LAT_errCode == LAT_RECEIVE_TIMEOUT)
	{
		LAT_errCode = LAT_HANDSHAKE_BGN_TIMEOUT_RX;
	}
	else
	{
		// do nothing
	}
}

void LAT_update_Txerr_bgn(void)
{
	if (LAT_errCode == LAT_TRANSMIT_WRONG_LENGTH)
	{
		LAT_errCode =  LAT_HANDSHAKE_BGN_LENGTH_TX;
	}
	else if (LAT_errCode == LAT_TRANSMIT_TIMEOUT)
	{
		LAT_errCode = LAT_HANDSHAKE_BGN_TIMEOUT_TX;
	}
	else
	{
		// do nothing
	}
}

void LAT_update_Rxerr_end(void)
{
	if (LAT_errCode == LAT_RECEIVE_WRONG_STRING)
	{
		LAT_errCode = LAT_HANDSHAKE_END_WRONG_RX;
	}
	else if (LAT_errCode == LAT_RECEIVE_TIMEOUT)
	{
		LAT_errCode = LAT_HANDSHAKE_END_TIMEOUT_RX;
	}
	else
	{
		// do nothing
	}
}

void LAT_update_Txerr_end(void)
{
	if (LAT_errCode == LAT_TRANSMIT_WRONG_LENGTH)
	{
		LAT_errCode =  LAT_HANDSHAKE_END_LENGTH_TX;
	}
	else if (LAT_errCode == LAT_TRANSMIT_TIMEOUT)
	{
		LAT_errCode = LAT_HANDSHAKE_END_TIMEOUT_TX;
	}
	else
	{
		// do nothing
	}
}

/*---- handshake ----*/

bool LAT_handshake_begin(void)
{
	if(LAT_wait_data(LAT_PC_TO_MCU_RDY, LAT_1ST_TIMEOUT_MS) == false)
	{
		LAT_update_Rxerr_bgn();
		return false;
	}

	if(LAT_send_data(LAT_MCU_TO_PC_ACK, LAT_TX_TIMEOUT_MS * strlen(LAT_MCU_TO_PC_ACK)) == false)
	{
		LAT_update_Txerr_bgn();
		return false;
	}

	if(LAT_wait_data(LAT_PC_TO_MCU_GO, LAT_RX_TIMEOUT_MS * strlen(LAT_PC_TO_MCU_GO)) == false)
	{
		LAT_update_Rxerr_bgn();
		return false;
	}
	else
		return true;
}

bool LAT_handshake_end(void)
{
	if(LAT_send_data(LAT_MCU_TO_PC_END, LAT_TX_TIMEOUT_MS * strlen(LAT_MCU_TO_PC_END)) == false)
	{
		LAT_update_Txerr_end();
		return false;
	}

	if(LAT_wait_data(LAT_MCU_TO_PC_ACK, LAT_RX_TIMEOUT_MS * strlen(LAT_MCU_TO_PC_ACK)) == false)
	{
		LAT_update_Rxerr_end();
		return false;
	}
	else
		return true;
}

/*---- the test for user logic ----*/

__weak bool LAT_logic_test(void)
{
	// re-define this function

	LAT_errCode = LAT_LOGICTEST_NONE;

	return false;
}

/*---- logic automatic tester flow ----*/

typLAT_errCode LAT_process_superLoop(void)
{
	HAL_UART_Receive_IT(LAT_UART_HANDLER,(uint8_t*) &LAT_rxInfo.rxByte, 1);

	if(LAT_handshake_begin() == false)
		return LAT_errCode;

	if(LAT_logic_test() == false)
	{
		if(LAT_errCode != LAT_LOGICTEST_NONE)
			LAT_errCode = LAT_LOGICTEST_FAILED;
	}

	LAT_handshake_end();

	return LAT_errCode;
}
