#pragma once

#include "main.h"

#define OW_DISABLE_IRQ()					__disable_irq();
#define OW_ENABLE_IRQ()						__enable_irq();
#define OW_NO_RESPOND_THRESHOLD_5msTick     (200u)                  // 1sec 동안 응답실패 현상 지속시, NO_RESPONSE 상태로 전이
#define OW_RDYCHK_EXTRA_5msTick             (2)                     // 준비상태 확인시, 타임아웃 추가 여유시간 (5ms 단위)
#define OW_CMDQ_SIZE                        (30u)                   // onewire 명령 큐 크기

#define TEMPSENS_9BIT_RESOL_WAIT_5msTick    (105u)                  // 9bit 해상도에서의 대기 시간 (5ms 단위)
#define TEMPSENS_10BIT_RESOL_WAIT_5msTick   (105u)                  // 10bit 해상도에서의 대기 시간 (5ms 단위)
#define TEMPSENS_11BIT_RESOL_WAIT_5msTick   (105u)                  // 11bit 해상도에서의 대기 시간 (5ms 단위)
#define TEMPSENS_12BIT_RESOL_WAIT_5msTick   (105u)                  // 12bit 해상도에서의 대기 시간 (5ms 단위)
#define TEMPSENS_DATA_INVALID_CNT_THRSLD    (5)                     // CRC 미스매치 연속 허용갯수
