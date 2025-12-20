/*--------------------------------------*/
// Author   : sungsoo
// Date     : 25.09.20
// Target   : STM32F103C8
/*--------------------------------------*/

#include "TaskSch_userTask.h"
#include "TaskSch_Config.h"
#include <stdint.h>

static typTask_timeInfo vTaskInfos[TASKSCH_NUMBER] = {0};

static inline void TaskSch_getCounter(uint32_t *paramTick)
{
	*paramTick = __HAL_TIM_GET_COUNTER(TASKSCH_TIM_HANDLER);
}

static void TaskSch_calExecTime(typTask_timeInfo *vTaskInfo)
{
	uint32_t startTick = vTaskInfo->typTempInfo.startTick;
	uint32_t endTick = vTaskInfo->typTempInfo.endTick;

	if(endTick >= startTick)
	{
		vTaskInfo->execTick = endTick - startTick;
	}
	else
	{
		vTaskInfo->execTick = endTick + (__HAL_TIM_GET_AUTORELOAD(TASKSCH_TIM_HANDLER) - startTick + 1);
	}
}

static void TaskSch_recordExecTime(typTask_timeInfo *vTaskInfo)
{
	TaskSch_calExecTime(vTaskInfo);

	if(vTaskInfo->execTick < vTaskInfo->minExecTick)
	{
		vTaskInfo->minExecTick = vTaskInfo->execTick;
	}

	if(vTaskInfo->execTick > vTaskInfo->maxExecTick)
	{
		vTaskInfo->maxExecTick = vTaskInfo->execTick;
	}
}

void TaskSch_initInfos(void)
{
	for(uint8_t id = 0; id < TASKSCH_NUMBER; id++)
	{
		vTaskInfos[id].execTick = 0;
		vTaskInfos[id].maxExecTick = 0;
		vTaskInfos[id].minExecTick = 0xffff;
		vTaskInfos[id].taskCnt = 0;
		vTaskInfos[id].typTempInfo.startTick = 0;
		vTaskInfos[id].typTempInfo.endTick = 0;
	}
}

void userTask00(void)
{
#ifdef TASKSCH_USE_MEASUERING
	TaskSch_getCounter(&vTaskInfos[TASKSCH_ID_00].typTempInfo.startTick);
#endif

	vTaskInfos[TASKSCH_ID_00].taskCnt++;

#ifdef TASKSCH_USE_MEASUERING
	TaskSch_getCounter(&vTaskInfos[TASKSCH_ID_00].typTempInfo.endTick);
	TaskSch_recordExecTime(&vTaskInfos[TASKSCH_ID_00]);
#endif
}

void userTask01(void)
{
#ifdef TASKSCH_USE_MEASUERING
	TaskSch_getCounter(&vTaskInfos[TASKSCH_ID_01].typTempInfo.startTick);
#endif
	
	vTaskInfos[TASKSCH_ID_01].taskCnt++;

#ifdef TASKSCH_USE_MEASUERING
	TaskSch_getCounter(&vTaskInfos[TASKSCH_ID_01].typTempInfo.endTick);
	TaskSch_recordExecTime(&vTaskInfos[TASKSCH_ID_01]);
#endif
}

void userTask02(void)
{
#ifdef TASKSCH_USE_MEASUERING
	TaskSch_getCounter(&vTaskInfos[TASKSCH_ID_02].typTempInfo.startTick);
#endif

	vTaskInfos[TASKSCH_ID_02].taskCnt++;

#ifdef TASKSCH_USE_MEASUERING
	TaskSch_getCounter(&vTaskInfos[TASKSCH_ID_02].typTempInfo.endTick);
	TaskSch_recordExecTime(&vTaskInfos[TASKSCH_ID_02]);
#endif
}

void userTask03(void)
{
#ifdef TASKSCH_USE_MEASUERING
	TaskSch_getCounter(&vTaskInfos[TASKSCH_ID_03].typTempInfo.startTick);
#endif

	vTaskInfos[TASKSCH_ID_03].taskCnt++;

#ifdef TASKSCH_USE_MEASUERING
	TaskSch_getCounter(&vTaskInfos[TASKSCH_ID_03].typTempInfo.endTick);
	TaskSch_recordExecTime(&vTaskInfos[TASKSCH_ID_03]);
#endif
}

void userTask04(void)
{
#ifdef TASKSCH_USE_MEASUERING
	TaskSch_getCounter(&vTaskInfos[TASKSCH_ID_04].typTempInfo.startTick);
#endif

	vTaskInfos[TASKSCH_ID_04].taskCnt++;

#ifdef TASKSCH_USE_MEASUERING
	TaskSch_getCounter(&vTaskInfos[TASKSCH_ID_04].typTempInfo.endTick);
	TaskSch_recordExecTime(&vTaskInfos[TASKSCH_ID_04]);
#endif
}
