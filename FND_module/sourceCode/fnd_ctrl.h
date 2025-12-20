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
#include "main.h"

#define FND_BIT_BANG_MODE 	(0)
#define FND_SPI_MODE		(1)

/*-------------------------------------------*/
// MODE SELECT
/*-------------------------------------------*/

#define FND_COMMU_MODE		(FND_SPI_MODE)

/*-------------------------------------------*/
// IRQ DEFINES
/*-------------------------------------------*/

#define FND_DISABLE_ISR()		__disable_irq();
#define FND_ENABLE_ISR()		__enable_irq();

/*-------------------------------------------*/
// GPIO PIN Configuration
/*-------------------------------------------*/

#if(FND_COMMU_MODE == FND_BIT_BANG_MODE)

/*----bitBang mode pin config----*/
#define SCLK_PORT 				FND_SCLK_GPIO_Port
#define RCLK_PORT				FND_RCLK_GPIO_Port
#define DIO_PORT				FND_DIO_GPIO_Port
#define SCLK_PIN				FND_SCLK_Pin
#define RCLK_PIN				FND_RCLK_Pin
#define DIO_PIN					FND_DIO_Pin

#else

/*----SPI mode pin config----*/
#define RCLK_PORT				FND_RCLK_GPIO_Port
#define RCLK_PIN				FND_RCLK_Pin

#endif
/*-------------------------------------------*/

#pragma pack(push,1)

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



#pragma pack(pop)

#if(FND_COMMU_MODE == FND_BIT_BANG_MODE)
bool fnd_init(void);
#else
bool fnd_init(SPI_HandleTypeDef* spiHandler);
#endif

bool fnd_setInteger(int16_t number);
bool fnd_setFloat(float data, eResoultion DPpos);
void fnd_printNumber_1ms(void);
void fnd_offLED(void);
void fnd_onLED(void);

#endif /* INC_FND_CTRL_H_ */
