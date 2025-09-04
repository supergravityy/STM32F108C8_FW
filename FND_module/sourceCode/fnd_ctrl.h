/*
 * fnd_ctrl.h
 *
 *  Created on: Aug 22, 2025
 *      Author: Smersh
 */

#ifndef INC_FND_CTRL_H_
#define INC_FND_CTRL_H_

#include <stdint.h>
#include <stdbool.h>

#define DIGITS_MAXCNT (4)
#define DECIMAL_MAXNUM (9)
#define DECIMAL_MAXCNT (10)

#define POS_INT_MAX (9999)
#define NEG_INT_MAX (999)

#define LED_OFF (0xFFFF)

typedef enum fndNumber
{
    FND_ZERO  = 0xC0,
    FND_ONE   = 0xF9,
    FND_TWO   = 0xA4,
    FND_THREE = 0xB0,
    FND_FOUR  = 0x99,
    FND_FIVE  = 0x92,
    FND_SIX   = 0x82,
    FND_SEVEN = 0xF8,
    FND_EIGHT = 0x80,
    FND_NINE  = 0x90,
	FND_OFF = 0xFF,
	FND_MINUS = 0xBF,
	FND_DP = 0x7F
} eFndNumber; // common annode 이기에 cathode는 low여야 점등됨

typedef enum digitNum
{
	FIRTST_PLACE,
	SECOND_PLACE,
	THIRD_PLACE,
	FORTH_PLACE
}edigitNum; // 모듈의 터미널핀과 가장 가까운 세그먼트부터 시작

typedef enum Resolution
{
	DECI_RESOL = THIRD_PLACE,
	CENTI_RESOL = SECOND_PLACE,
	MILLI_RESOL = FIRTST_PLACE
}eResoultion;

typedef enum DPset
{
	FND_DP_CLR = 0,
	FND_DP_SET = 1
}eDPset;

typedef enum LED_OnOff
{
	FND_LED_OFF = 0,
	FND_LED_ON = 1
}eLED_OnOff;

typedef enum Num_PosNeg
{
	FND_NUMBER = 0,
	FND_NEG_CHAR = 1
}typNum_PosNeg;

typedef struct segement
{
	uint8_t selectBits;			// 모듈의 seg선택비트
	uint8_t printData;			// segment 전송 데이터
	uint8_t deciNum;			// segment 데이터의 의미
	eDPset deciPnt;				// 소수점 포함여부 -> 4개의 seg중 하나만
	typNum_PosNeg segChar_stat;	// 모듈이 표현할 수가 음수부호인지 -> 4개의 seg중 하나만
	eLED_OnOff OnOff_stat;		// LED onoff 여부
}typSegment;

typedef struct printNumber
{
	uint8_t deciNum;			// 객체에게 넘겨줄 데이터
	eDPset deciPnt;				// 소수점 포함여부 -> 4개의 seg중 하나만
	eLED_OnOff OnOff_stat;		// LED onoff 여부
	typNum_PosNeg segChar_stat;	// 모듈이 표현할 수가 음수부호인지 -> 4개의 seg중 하나만
}typPrintNum;

extern void fnd_init(void);
extern bool fnd_setInteger(int16_t number);
extern bool fnd_setFloat(float data, eResoultion DPpos);
void fnd_printNumber_1ms(void);
extern void fnd_offLED(void);
extern void fnd_onLED(void);

// extern void fnd_dataSend(uint8_t data, edigitNum digNum);

#endif /* INC_FND_CTRL_H_ */
