/*
 * fnd_ctrl.c
 *
 *  Created on: Aug 25, 2025
 *      Author: Smersh
 */

#include "fnd_ctrl.h"
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define DIGITS_MAXCNT (4)
#define DECIMAL_MAXNUM (9)
#define DECIMAL_MAXCNT (10)

#define POS_INT_MAX (9999)
#define NEG_INT_MAX (999)

#define LED_OFF (0xFFFF)

#define EXTRACT_MSB(data) ((data) & 0x80)
#define CHECK_POS_INT_SCOPE(num) (((num) >= 0) && ((num) <= POS_INT_MAX))
#define CHECK_NEG_INT_SCOPE(num) (((num) < 0) && ((num) >= -NEG_INT_MAX))

#pragma pack(push,1)

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

#if(FND_COMMU_MODE == FND_BIT_BANG_MODE)
/*----bitBang mode control obj----*/

typedef struct fnd_obj
{
	typSegment segmentObj[DIGITS_MAXCNT];
	uint32_t fnd_count;
}typFND_bitBang_obj;

typFND_bitBang_obj vFnd_obj;

#else


/*----SPI mode control obj----*/

typedef struct fnd_obj
{
	SPI_HandleTypeDef* spiHandler;
	typSegment segmentObj[DIGITS_MAXCNT];
	uint32_t fnd_count;
}typFND_Spi_obj;

typFND_Spi_obj vFnd_obj;

#endif

#pragma pack(pop)

static volatile eLED_OnOff fnd_LEDstatFlag = FND_LED_ON; // 컴파일러 최적화로 변수 삭제 방지
static typPrintNum number_buff[DIGITS_MAXCNT];

static const uint8_t fnd_dataExamples [DECIMAL_MAXCNT] = {FND_ZERO, FND_ONE, FND_TWO,
		FND_THREE, FND_FOUR, FND_FIVE, FND_SIX, FND_SEVEN, FND_EIGHT, FND_NINE};

/*----------------------------------------*/
// Control HW
/*----------------------------------------*/
#if(FND_COMMU_MODE == FND_BIT_BANG_MODE)
static void fnd_Update_serialInput_FF(void)
{
	// high->low 만 보장하기에 첫 상태로 인한 변화 보장 안될수도
	HAL_GPIO_WritePin(SCLK_PORT, SCLK_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SCLK_PORT, SCLK_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SCLK_PORT, SCLK_PIN, GPIO_PIN_RESET);
}

static void fnd_blankAll(void)
{
	fnd_send_8Bit(0xFF);    // 세그 OFF
	fnd_send_8Bit(0x00);    // 자리 OFF
	fnd_Update_parallelOutput_FF();
}

static void fnd_send_8Bit(uint8_t data4send)
{
	for(int8_t i = 8; i > 0; i--)
	{
		if (EXTRACT_MSB(data4send))
			HAL_GPIO_WritePin(DIO_PORT, DIO_PIN, GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(DIO_PORT, DIO_PIN, GPIO_PIN_RESET);

		data4send <<= 1;
		fnd_Update_serialInput_FF();
	}
}
#else
static void fnd_send_8Bit(uint8_t data4send)
{
	HAL_SPI_Transmit(vFnd_obj.spiHandler, &data4send, 1, 1);
}

static void fnd_blankAll(void)
{
	uint8_t off_allLED = 0xff;
	uint8_t off_allPos = 0x0;
	HAL_SPI_Transmit(vFnd_obj.spiHandler, &off_allLED, 1, 1);
	HAL_SPI_Transmit(vFnd_obj.spiHandler, &off_allPos, 1, 1);
}
#endif

static void fnd_Update_parallelOutput_FF(void)
{
	HAL_GPIO_WritePin(RCLK_PORT, RCLK_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(RCLK_PORT, RCLK_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(RCLK_PORT, RCLK_PIN, GPIO_PIN_RESET);
}

/*----------------------------------------*/
// Init
/*----------------------------------------*/

#if(FND_COMMU_MODE == FND_BIT_BANG_MODE)
/*----bitBang mode init----*/
bool fnd_init(void)
{
	for(int i = 0; i < DIGITS_MAXCNT; i++)
	{
		// 1. selectBits 멤버 초기화
		vFnd_obj.segmentObj[i].selectBits =  (1 << (FORTH_PLACE - i));

		// 2. 나머지는 main함수에서 fnd_printNumber_1ms가 number_buff 값을 받아감
		number_buff[i].OnOff_stat = FND_LED_OFF;
		number_buff[i].deciNum = 0;
		number_buff[i].segChar_stat = FND_NUMBER;
		number_buff[i].deciPnt = FND_DP_CLR;
	}

	// 3. clk 상태 초기화 -> 첫 비트의 상승엣지 보장
	HAL_GPIO_WritePin(SCLK_PORT, SCLK_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(RCLK_PORT, RCLK_PIN, GPIO_PIN_RESET);

	// 4. led on
	fnd_onLED();

	return true;
}
#else
/*----spi mode init----*/
bool fnd_init(SPI_HandleTypeDef* spiHandler)
{
	bool result;

	if(spiHandler == NULL)
	{
		result = false;
	}
	else
	{
		vFnd_obj.spiHandler = spiHandler;

		for (int i = 0; i < DIGITS_MAXCNT; i++)
		{
			// 1. selectBits 멤버 초기화
			vFnd_obj.segmentObj[i].selectBits = (1 << (FORTH_PLACE - i));

			// 2. 나머지는 main함수에서 fnd_printNumber_1ms가 number_buff 값을 받아감
			number_buff[i].OnOff_stat 		= FND_LED_OFF;
			number_buff[i].deciNum 			= 0;
			number_buff[i].segChar_stat 	= FND_NUMBER;
			number_buff[i].deciPnt 			= FND_DP_CLR;
		}

		// 3. clk 상태 초기화 -> 첫 비트의 상승엣지 보장
		HAL_GPIO_WritePin(RCLK_PORT, RCLK_PIN, GPIO_PIN_RESET);

		// 4. led on
		fnd_onLED();

		result = true;
	}

	return result;
}
#endif

/*----------------------------------------*/
// Task functions
/*----------------------------------------*/

static void fnd_cnvrt_digitBits(typSegment* inputData) // 1의자리 숫자 -> 7segment로 변경
{
	uint8_t result = 0x0;

	if(inputData->OnOff_stat == FND_LED_OFF)		// 1. LED off인지 확인
	{
		result = FND_OFF;
	}
	else if(inputData->segChar_stat == FND_NEG_CHAR)// 2. 음수 부호인지 확인
	{
		result = FND_MINUS;
	}
	else											// 3. 숫자표현 및 소수점표현
	{
		result = fnd_dataExamples[inputData->deciNum];

		(inputData->deciPnt) ? (result &= FND_DP) : (result |= 0x00);
	}

	inputData->printData = result;
}

void fnd_printNumber_1ms(void)
{
	FND_DISABLE_ISR(); 										// 1. number_buff의 무결성 보장 start

	for(uint8_t i = 0; i < DIGITS_MAXCNT; i++) 				// 2. 임시버퍼 내용 복사
	{
		vFnd_obj.segmentObj[i].deciNum = number_buff[i].deciNum;
		vFnd_obj.segmentObj[i].deciPnt = number_buff[i].deciPnt;
		vFnd_obj.segmentObj[i].OnOff_stat = number_buff[i].OnOff_stat;
		vFnd_obj.segmentObj[i].segChar_stat = number_buff[i].segChar_stat;
		fnd_cnvrt_digitBits(&vFnd_obj.segmentObj[i]);
	}

	FND_ENABLE_ISR(); 										// 3. number_buff의 무결성 보장 end

	for(int8_t i = FORTH_PLACE; i >= FIRTST_PLACE; i--) 	// 4. 객체 정보대로 data 전송
	{
		fnd_blankAll(); // 잔상 줄이기

		fnd_send_8Bit(vFnd_obj.segmentObj[i].printData);
		fnd_send_8Bit(vFnd_obj.segmentObj[i].selectBits);
		fnd_Update_parallelOutput_FF();	// 출력 확정
	}

	vFnd_obj.fnd_count++;
}


/*----------------------------------------------------*/
// user API implementation
/*----------------------------------------------------*/

static void fnd_fill_intDigits(uint16_t number)
{
	// 1. 버퍼 초기화
	for (int8_t i = FORTH_PLACE; i >= FIRTST_PLACE; i--)
	{
		number_buff[i].deciNum = 0;
		number_buff[i].OnOff_stat = fnd_LEDstatFlag;
		number_buff[i].deciPnt = FND_DP_CLR;
		number_buff[i].segChar_stat = FND_NUMBER;
	}

	// 2. 버퍼에 값 채우기
	number_buff[FORTH_PLACE].deciNum = (uint8_t)(number % 10);
	number_buff[THIRD_PLACE].deciNum = (uint8_t)((number / 10) % 10);
	number_buff[SECOND_PLACE].deciNum = (uint8_t)((number / 100) % 10);
	number_buff[FIRTST_PLACE].deciNum = (uint8_t)(number / 1000);
	// 음수일때의 경우에서는 문제 X -> 최좌측의 수보다 부호를 먼저 확인하기에 상관 없음
}
bool fnd_setInteger(int16_t data)
{
	bool result = false;

	if(CHECK_POS_INT_SCOPE(data))					// 1. 양수일때
	{
		fnd_fill_intDigits(data);					// 1.1 버퍼 업데이트

		result = true;
	}

	else if (CHECK_NEG_INT_SCOPE(data))				// 2. 음수일때
	{
		fnd_fill_intDigits(-data);					// 2.2 버퍼 업데이트

		number_buff[FIRTST_PLACE].segChar_stat = FND_NEG_CHAR;

		result = true;
	}

	else // 3. 이상한 수를 받았을 때
	{
		result = false;
	}

	return result;
}
bool fnd_setFloat(float data, eResoultion DPpos)
{
    bool result = false;
    int32_t scaled = 0;

    switch (DPpos)					// 1. 소수점에 맞게 스케일링하여 정수화
    {
    case DECI_RESOL:   scaled = (int32_t)(data * 10);   break;
    case CENTI_RESOL:  scaled = (int32_t)(data * 100);  break;
    case MILLI_RESOL:  scaled = (int32_t)(data * 1000); break;
    default: return false;
    }

    if (CHECK_POS_INT_SCOPE(scaled)) // 2. 양수/음수에 따라
    {
        fnd_fill_intDigits((uint16_t)scaled); // 2.1 버퍼 업데이트
        result = true;
    }
    else if (CHECK_NEG_INT_SCOPE(scaled) && (DPpos != MILLI_RESOL))
    {
        fnd_fill_intDigits((uint16_t)(-scaled));
        number_buff[FIRTST_PLACE].segChar_stat = FND_NEG_CHAR;
        result = true;
    }
    else
    {
        result = false;
    }

    if (result)						// 3. 성공하였을 때, 소수점 붙이기
        number_buff[DPpos].deciPnt = FND_DP_SET;

    return result;
}
void fnd_offLED(void)
{
	fnd_LEDstatFlag = FND_LED_OFF;
}
void fnd_onLED(void)
{
	fnd_LEDstatFlag = FND_LED_ON;
}
