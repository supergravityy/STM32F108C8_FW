#pragma once

#include "main.h"
#include "TempSensor_Config.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum errCodeOW // onewire -> tempsens
{
    OW_ERR_OKAY                     = (uint8_t)0,       // 0. 정상상태
    OW_ERR_NOT_INITIALIZED          = (uint8_t)1,       // 1. 초기화 되지 않은 상태로 상태머신이 돌아감
    OW_ERR_ALREADY_INITIALIZED      = (uint8_t)2,       // 2. 이미 초기화된 상태에서 다시 초기화 시도
    OW_ERR_CANT_INITIALIZE          = (uint8_t)3,       // 3. 초기화 불가 -> 파라미터 오류
    OW_ERR_NOT_RESPONDING           = (uint8_t)4,       // 4. DIO선이 high 상태로 1초간 고정 -> 장치 초기화 실패
    OW_ERR_NO_RESPONSE    			= (uint8_t)5,       // 5. DIO선이 일정시간 high 상태로 고정 -> 장치 응답없음
    OW_ERR_CMD_INVALID              = (uint8_t)6,       // 6. 유효하지 않은 명령어가 큐에 들어옴
    OW_ERR_TIMEOUT                  = (uint8_t)7,       // 7. ECU의 작업 준비시간 초과
}typErrCodeOW;

typedef enum statesOW
{
    OW_STATE_START                  = (uint8_t)0,       // OW 상태머신이 가지는 첫 상태
    OW_STATE_IDLE                   = (uint8_t)1,       // 대기 상태
    OW_STATE_RESET                  = (uint8_t)2,       // 리셋 상태
    OW_STATE_WRITE                  = (uint8_t)3,       // 쓰기 상태
    OW_STATE_READ                   = (uint8_t)4,       // 읽기 상태
    OW_STATE_RDY_CHK                = (uint8_t)5,       // 준비상태 확인 상태
    OW_STATE_RECOVERY               = (uint8_t)6,       // 통신라인 복구 상태
    OW_STATE_ERROR                  = (uint8_t)7        // 에러 상태
}typStatesOW;

typedef enum cmdOW
{
    OW_CMD_IDLE     = (uint8_t)0,                       // 대기상태
    OW_CMD_RESET    = (uint8_t)1,                       // 리셋 명령 -> ecu에 리셋신호 전송
    OW_CMD_WRITE    = (uint8_t)2,                       // 쓰기 명령 -> 1바이트를 전송
    OW_CMD_READ     = (uint8_t)3,                       // 읽기 명령 -> 1바이트를 수신
    OW_CMD_RDY_CHK  = (uint8_t)4,                       // 준비상태 확인 명령 -> 일정시간동안 1비트가 LOW로 떨어지는지를 체크
    OW_CMD_STOP     = (uint8_t)5                        // 작업 중지 명령 -> 이전 작업을 취소하고 큐를 모두 비우기
}typCmdOW;

/*----user API----*/
void oneWire_cmdQueue_enQueue(typCmdOW cmd, uint8_t* param); // 큐의 맨 뒤에 명령어 추가
typErrCodeOW oneWire_getErrCode(void); 
uint8_t oneWire_getCmdQ_reservNum(void);
bool oneWire_isReady_forGettingCmd(void); // 상위계층에서 enqueue를 할 준비가 되었는지를 확인가능
typStatesOW oneWire_getState(void);
void oneWire_stMachine_init(GPIO_TypeDef *GPIOx, uint16_t DIO_Pin, TIM_HandleTypeDef *htimX);
void oneWire_process_5ms(void); 
