/*--------------------------------------*/
// Author   : sungsoo
// Date     : 25.09.20
// Target   : STM32F103C8
/*--------------------------------------*/

#include "TaskSch.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef TASKSCH_USE_ASSERT
#define CRIT_ERR()\
{while(1){}}
#else
#define CRIT_ERR() {}
#endif

static typSchTask vSchTask_List[TASKSCH_NUMBER];

static void TaskSch_config(uint8_t taskId, uint16_t period_ms, uint16_t phase_ms,
		void (*taskFunc_ptr)(void));

void TaskSch_init(void)
{
	TaskSch_initInfos();

    // 1ms task
    TaskSch_config(TASKSCH_ID_00, 1, 0, userTask00);
    TaskSch_config(TASKSCH_ID_01, 1, 10, userTask01);

    // 5ms task
    TaskSch_config(TASKSCH_ID_02, 5, 0, userTask02);

    // 100ms task
    TaskSch_config(TASKSCH_ID_03, 100, 0, userTask03);

    // 1s task
    TaskSch_config(TASKSCH_ID_04, 1000, 0, userTask04);
}

static void TaskSch_config(uint8_t taskId, uint16_t period_ms, uint16_t phase_ms,
		void (*taskFunc_ptr)(void))
{
    if (taskId < TASKSCH_NUMBER || taskFunc_ptr == NULL)
    {
        vSchTask_List[taskId].period_ms = period_ms;
        vSchTask_List[taskId].phase_ms = phase_ms;
        vSchTask_List[taskId].taskFunc_ptr = taskFunc_ptr;
        vSchTask_List[taskId].activation = false;
    }
    else
    {
        CRIT_ERR();
    }
}

void TaskSch_execTask(void)
{
	uint8_t idx = 0;

	for(; idx < TASKSCH_NUMBER; idx++)
	{
		if(vSchTask_List[idx].activation == true)
		{
			vSchTask_List[idx].taskFunc_ptr();

            DISABLE_ISR();
			vSchTask_List[idx].activation = false;
            ENABLE_ISR();
		}
		else
		{
			// do nothing
		}
	}
}

void TaskSch_timeManager(void)
{
    uint8_t idx = 0;

    for (;idx < TASKSCH_NUMBER; idx++)
    {
        if(vSchTask_List[idx].phase_ms > 0)
        {
            vSchTask_List[idx].phase_ms--;
        }
        else
        {
            vSchTask_List[idx].curTick_ms++;

            if(vSchTask_List[idx].curTick_ms == vSchTask_List[idx].period_ms)
            {
                vSchTask_List[idx].activation = true;
                vSchTask_List[idx].curTick_ms = 0;
            }
            else
            {
            	// do nothing
            }
        }
    }
}
