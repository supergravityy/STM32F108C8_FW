#include <stdint.h>
#include <stdbool.h>
#include "CRC8.h"

/*--------------------------------*/
// include User Libs START
/*--------------------------------*/

/*--------------------------------*/
// include User Libs END
/*--------------------------------*/

#define CRC8_POLY_WITH_TOP_SHIFTED		(0x107 << 7)  // 0x8380
#define CRC8_CHK_MSB(window)			(window & 0x8000)
#define CRC8_GET_1BIT(byte,pos)			((byte >> pos) & 0x01) << 7
#define CRC8_FILL_WINDOW(window,bit1)	(uint16_t)((window << 1) | (uint16_t)bit1)
#define CRC8_GET_CRC_AT_WINDOW(window)	(uint8_t)((window >> 7) & 0xff)
#define CRC8_CHK_INTEGRITY(window)		(window == 0)

/*--------------------------------*/
// User defines START
/*--------------------------------*/

/*--------------------------------*/
// User defines END
/*--------------------------------*/

/*--------------------------------------------*/
// 함수 기능 : CRC-8 계산
// 입력 파라미터 : 패킷 주소, 패킷길이
// 특이사항 : 패킷의 주소의 맨 마지막은 0으로 채워져 있을 것
// 반환 값 : 계산된 CRC-8 값
/*--------------------------------------------*/

uint8_t CRC8_calcCRC(const uint8_t* pcktData, uint8_t pcktLength)
{
	uint16_t bitWindow = 0;
	uint8_t oneBit = 0, a_Byte;

	// 패킷을 다항식으로 나누기(XOR 연산)
	for (uint8_t byteIdx = 0; byteIdx < pcktLength; byteIdx++)	// 바이트 추출 반복문
	{
		a_Byte = pcktData[byteIdx];

		for (int8_t bitPos = 7; bitPos >= 0; bitPos--)			// 비트 추출 반복문
		{
			oneBit = CRC8_GET_1BIT(a_Byte, bitPos);
			bitWindow = CRC8_FILL_WINDOW(bitWindow, oneBit);

			if (CRC8_CHK_MSB(bitWindow))
				bitWindow ^= CRC8_POLY_WITH_TOP_SHIFTED;
		}
	}

	return CRC8_GET_CRC_AT_WINDOW(bitWindow);
}

/*--------------------------------------------*/
// 함수 기능 : 패킷이 무결성을 만족하는 지 확인
// 입력 파라미터 : 패킷 주소, 패킷길이
// 특이사항 : XOR 연산 후 나머지가 0이면 무결성 만족
// 반환 값 : T/F
/*--------------------------------------------*/

bool CRC8_chkIntegrity(const uint8_t* pcktData, uint8_t pcktLength)
{
	uint16_t bitWindow = 0;
	uint8_t oneBit = 0, a_Byte;

	// 패킷을 다항식으로 나누기(XOR 연산)
	for (uint8_t byteIdx = 0; byteIdx < pcktLength; byteIdx++)	// 바이트 추출 반복문
	{
		a_Byte = pcktData[byteIdx];

		for (int8_t bitPos = 7; bitPos >= 0; bitPos--)			// 비트 추출 반복문
		{
			oneBit = CRC8_GET_1BIT(a_Byte, bitPos);
			bitWindow = CRC8_FILL_WINDOW(bitWindow, oneBit);

			if (CRC8_CHK_MSB(bitWindow))
				bitWindow ^= CRC8_POLY_WITH_TOP_SHIFTED;
		}
	}
	return CRC8_CHK_INTEGRITY(bitWindow);
}

void CRC8_fillPckt(void* packet)
{
    /*--------------------------------*/
    // User Code 
    // Goal : Packet must be intialized for calculating CRC8 value
    // Caution : Last byte must be filled by 0x00 
    /*--------------------------------*/
}