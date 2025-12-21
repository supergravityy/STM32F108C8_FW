#pragma once 

#include "main.h"
#include "OW.h"
#include "TEMPSENSOR_Config.h"
#include <stdint.h>
#include <stdbool.h>

#define TEMPSENS_SCRATCHPAD_SIZE        (9)

#pragma pack(push, 1)

typedef enum Cmd_tempSens // 유저가 직접적으로 호출하는 명령집합
{
    TEMPSENS_CMD_WAIT,
    TEMPSENS_CMD_REQ_DATA,
    TEMPSENS_CMD_SET_RESOL,
    TEMPSENS_CMD_SAVE_RESOL,
    TEMPSENS_CMD_RECOV_PREV_RESOL,
    TEMPSENS_CMD_STOP_ALL_PROCESS
}typCmd_tempSens;

typedef enum tempSensor_errCode // tempsens -> user
{
    TEMPSENS_ERR_OKAY,              // 정상동작
    TEMPSENS_ERR_NOT_INITIALIZED,   // 초기화 되지 않은 상태로 상태머신이 동작함
    TEMPSENS_ERR_ALREADY_INIT,      // 초기화가 2번이상 이루어짐
    TEMPSENS_ERR_CANT_INITIALIZE,   // 초기화 불가능
    TEMPSENS_ERR_NO_RESPONSE ,      // 센서와의 단선상황
    TEMPSENS_ERR_INVALID_CMD,       // 유효하지않은 명령수신
    TEMPSENS_ERR_MALFUNC,           // CRC 미스매치가 너무 많이 이루어짐
    TEMPSENS_ERR_TIMEOUT            // 센서가 명령에 응답하지 않음
}typTempSens_errCode; 

#pragma pack(pop)

/*-----user API-----*/
void tempSens_stMachine_init(void);
void tempSensor_100ms(void);
bool tempSens_isReady_forReq(void);
bool tempSensor_reqCommand(typCmd_tempSens cmd, uint8_t *param, bool* reqFailed); 
float tempSensor_getTemper_celcius(void);
float tempSensor_getTemper_fahrenheit(void);
uint8_t tempSensor_getResolution(void);
typTempSens_errCode tempSensor_getErrCode(void);

