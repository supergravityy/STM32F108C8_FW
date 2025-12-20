/*--------------------------------------*/
// Author   : sungsoo
// Date     : 25.09.20
// Target   : STM32F103C8
/*--------------------------------------*/

#pragma once

#include "TaskSch_Config.h"
#include "TaskSch_userTask.h"
#include <stdbool.h>
#include <stdint.h>

#pragma pack(push,1)

typedef struct SchTask
{
    uint8_t taskId;
    void (*taskFunc_ptr)(void);

    uint16_t period_ms;
    uint16_t phase_ms;
    volatile uint32_t curTick_ms;
    
    volatile bool activation;
}typSchTask;

typedef struct execTimer
{
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint16_t mili_sec;
}typExecTimer;

#pragma pack(pop)

extern typExecTimer TaskSch_execClock;

// bool TaskSch_config(uint8_t taskId, uint16_t period_ms, uint16_t phase_ms, bool activation);
extern void TaskSch_init(void);
extern void TaskSch_timeManager(void);
extern void TaskSch_execTask(void);
extern void TaskSch_updateExecClock(void);
