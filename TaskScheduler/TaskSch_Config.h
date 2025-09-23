/*--------------------------------------*/
// Author   : sungsoo
// Date     : 25.09.20
// Target   : STM32F103C8
/*--------------------------------------*/

# pragma once

#include "main.h"

extern TIM_HandleTypeDef htim2;

// Select whether to apply a syntax that allows you
// to proactively detect the configuration when it is incorrect
// OPTIONAL
#define TASKSCH_USE_ASSERT

// Select whether to check the running time of all tasks
// OPTIONAL
#define TASKSCH_USE_MEASUERING

// Please enter the macro to pause and resume the ISR
// ESSENTIAL
#define DISABLE_ISR()   		__disable_irq();
#define ENABLE_ISR()    		__enable_irq();

// Please enter a timer object that has the criteria for the TASK cycle
// ESSENTIAL
#define TASKSCH_TIM_HANDLER		(&htim2)

// Please enter how many TASKs you want to make
// ESSENTIAL
#define TASKSCH_NUMBER          (5)

// Please enter the TASK's ID number. this number will be used by indexing TASK array
// ESSENTIAL
#define TASKSCH_ID_00           (0)
#define TASKSCH_ID_01           (1)
#define TASKSCH_ID_02           (2)
#define TASKSCH_ID_03           (3)
#define TASKSCH_ID_04           (4)
