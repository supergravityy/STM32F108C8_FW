/*--------------------------------------*/
// Author   : sungsoo
// Date     : 25.09.20
// Target   : STM32F103C8
/*--------------------------------------*/

#pragma once

#include <stdint.h>
#include "TaskSch_Config.h"

typedef struct task_timeInfo
{
	uint32_t taskCnt;
	uint16_t execTick;
	uint16_t maxExecTick;
	uint16_t minExecTick;

	struct tempInfo
	{
		uint32_t startTick;
		uint32_t endTick;
	}typTempInfo;
}typTask_timeInfo;

extern void TaskSch_initInfos(void);

extern void userTask00(void);
extern void userTask01(void);
extern void userTask02(void);
extern void userTask03(void);
extern void userTask04(void);
