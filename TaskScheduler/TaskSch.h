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

typedef struct SchTask
{
    uint8_t taskId;
    void (*taskFunc_ptr)(void);

    uint16_t period_ms;
    uint16_t phase_ms;
    volatile uint32_t curTick_ms;
    
    volatile bool activation;
}typSchTask;

// bool TaskSch_config(uint8_t taskId, uint16_t period_ms, uint16_t phase_ms, bool activation);
extern void TaskSch_init(void);
extern void TaskSch_timeManager(void);
extern void TaskSch_execTask(void);
