#include "OW.h"
#include "TempSensor_Config.h"
#include "TempSensor.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TEMPSENS_CHK_VALID_CMD(userCMD) ((TEMPSENS_CMD_WAIT <= userCMD) && (userCMD <= TEMPSENS_CMD_STOP_ALL_PROCESS))
#define TEMPSENS_CHK_RESOL_RANGE(resol) ((resol < 9) && (resol > 12))
// Scratchpad resolution bits
#define TEMPSENS_CONFIG_RESOL_R1 (1 << 6)
#define TEMPSENS_CONFIG_RESOL_R0 (1 << 5)
// Scratchpad byte positions
#define TEMPSENS_TEMP_LSB_POS (0)
#define TEMPSENS_TEMP_MSB_POS (1)
#define TEMPSENS_ALARM_HIGH_POS (2)
#define TEMPSENS_ALARM_LOW_POS (3)
#define TEMPSENS_RESOL_POS (4)
#define TEMPSENS_CRC_POS (8)
// Scratchpad init VAL
#define TEMPSENS_PAD_INIT_VAL (0xFF)
// function
#define TEMPSENS_CEL2FAH(cel) (float)((9.0 / 5 * cel) + 32)
// cmd idx num
#define TEMPSENS_CMDIDX_ARR_SIZE (7)
#define TEMPSENS_CMDIDX_SKIPROM (0)
#define TEMPSENS_CMDIDX_CONVERT (1)
#define TEMPSENS_CMDIDX_READ (2)
#define TEMPSENS_CMDIDX_WRITE (3)
#define TEMPSENS_CMDIDX_SAVE (4)
#define TEMPSENS_CMDIDX_RECALL (5)
#define TEMPSENS_CMDIDX_STOP (6)

#pragma pack(push,1)

typedef enum tempSensor_states
{
    TEMPSENS_STATE_START,       // 초기상태
    TEMPSENS_STATE_IDLE,        // 초기화를 마친 후 대기상태
    TEMPSENS_STATE_EXEC,        // 명령을 수행 중
    TEMPSENS_STATE_PENDING,     // 읽기완료 대기중
    TEMPSENS_STATE_BUSY,        // 선 복구 혹은 명령 큐가 꽉차서 명령 수행불가능 상태
    TEMPSENS_STATE_ERROR        // 에러상태
}typTempSens_st;

typedef struct tempSensCMD
{
    typCmd_tempSens cmd;
    uint8_t* param;
    bool* reqFailed;
}typTempSensCMD;

typedef enum tempSens_provCMDcodes // 데이터시트에 명기된 명령코드
{
    TEMPSENS_CMD_SKIP_ROM_FIND = (uint8_t)(0xCC),
    TEMPSENS_CMD_CONVERT_TEMPER = (uint8_t)(0x44),
    TEMPSENS_CMD_READ_SCRATCHPAD = (uint8_t)(0xBE),
    TEMPSENS_CMD_WRT_SCRATCHPAD = (uint8_t)(0x4E),
    TEMPSENS_CMD_CPY_SCRATCHPAD = (uint8_t)(0x48),
    TEMPSENS_CMD_RECALL_EEPROM = (uint8_t)(0xB8)
} typTempSens_provCMDcodes;

typedef struct tempSensor_obj
{
    // 온도센서로부터 읽어들인 값
    uint8_t scratchpad[TEMPSENS_SCRATCHPAD_SIZE];
    float temperCelcius;                // 센서가 최근에 읽어들인 온도정보
    uint8_t sensorResol;                // 센서에 현재 지정된 분해능값
    uint8_t resol8Bit;                  // 사용자가 지정한 분해능을 센서 레지스터에 맞게 비트연산을 한 멤버
    bool scratchpadFilled;              // 스크래치패드에 9바이트가 전부 들어왔는지 체크
    bool lastData_IsValid_flag;         // 마지막으로 읽어들인 데이터의 유효성 여부 -> CRC
    bool stopProc;                      // 작업종료 명령 수신여부
    
    // 상태머신 메타데이터
    typTempSens_errCode errCode;        // 에러코드
    uint8_t mismatch_dataCnt;           // 연속 crc 미스매치 횟수
    typTempSens_st currState;
    typTempSens_st prevState;
    typStatesOW OWcurrState;
    typErrCodeOW OWerrCode;
    bool initialized;
    uint32_t stateCnt;

    // 사용자로부터 받는 명령 큐
    typTempSensCMD currCmd;
}typTempSensor_obj; // 실질적인 온도센서 객체

// test
extern void TempSensor_Dummy_TC03_1(uint8_t* target);

typedef struct tempSens_resolInfo
{
    uint8_t resolBit;
    float resolution;
    uint8_t conv5msTick;
} typTempSens_resolInfo;

typedef struct tempSens_CMDqueueInfo
{
    typTempSens_provCMDcodes cmdCode;
    uint8_t OWcmdNum;
    bool (*cmdFunc)(void);
} typTempSens_CMDqueueInfo;

#pragma pack(pop)

static bool tempSensor_convTemper(void);
static bool tempSensor_readScratchPad(void);
static bool tempSensor_setResolution(void);
static bool tempSensor_saveCurrResol(void);
static bool tempSensor_recoverPrevResol(void);
static bool tempSensor_stopAllProcess(void);

const typTempSens_resolInfo vTempSens_resolInfo[4] =
    {{(uint8_t)9u, 0.5f, (uint8_t)TEMPSENS_9BIT_RESOL_WAIT_5msTick}, {(uint8_t)10u, 0.25f, (uint8_t)TEMPSENS_10BIT_RESOL_WAIT_5msTick}
    , {(uint8_t)11u, 0.125f, (uint8_t)TEMPSENS_11BIT_RESOL_WAIT_5msTick}, {(uint8_t)12u, 0.0625f, (uint8_t)TEMPSENS_12BIT_RESOL_WAIT_5msTick}};

const typTempSens_CMDqueueInfo vTempSens_cmdQInfo[TEMPSENS_CMDIDX_ARR_SIZE] =
    {{TEMPSENS_CMD_SKIP_ROM_FIND, 1, NULL}, {TEMPSENS_CMD_CONVERT_TEMPER, 17, tempSensor_convTemper}, {TEMPSENS_CMD_READ_SCRATCHPAD, 13, tempSensor_readScratchPad}, 
    {TEMPSENS_CMD_WRT_SCRATCHPAD, 7, tempSensor_setResolution}, {TEMPSENS_CMD_CPY_SCRATCHPAD, 3, tempSensor_saveCurrResol}, 
    {TEMPSENS_CMD_RECALL_EEPROM, 3, tempSensor_recoverPrevResol}, {0, 0, tempSensor_stopAllProcess}};

static typTempSensor_obj vTempSensor_obj;

/*
 * INFO : oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_SKIPROM].cmdCode); 의 이유
 *
 * 1. 인큐는 기본적으로 읽기와 쓰기명령을 구분하지 않고 저장한다.
 * 2. vTempSens_cmdQInfo 배열안의 내용은 불변이어야 한다.
 * 3. oneWire계층에서 사용하는 방식은 이미 안전하게 사용하게끔 정해져 있다.
 * -> const uint8_t * 를 uint8_t 로 형변환 해서 전달하는 이유
 * */

/*--------------------------------------------*/
// Command Enqueue Processing Functions
/*--------------------------------------------*/
static bool tempSensor_convTemper(void) // 명령갯수 -> 17
{
	uint8_t idx;

    if (!oneWire_isReady_forGettingCmd() ||
        oneWire_getCmdQ_reservNum() < vTempSens_cmdQInfo[TEMPSENS_CMDIDX_CONVERT].OWcmdNum)
    {
        return false;
    }

    oneWire_cmdQueue_enQueue(OW_CMD_RESET, NULL);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_SKIPROM].cmdCode);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_CONVERT].cmdCode);

    if(vTempSensor_obj.sensorResol == 0)
    {
    	oneWire_cmdQueue_enQueue(OW_CMD_RDY_CHK, NULL);
    }
    else
    {
		for (idx = 0; idx < 4; idx++)
		{
			if (vTempSensor_obj.sensorResol == vTempSens_resolInfo[idx].resolBit)
				break;
		}

		oneWire_cmdQueue_enQueue(OW_CMD_RDY_CHK, (uint8_t*) &vTempSens_resolInfo[idx].conv5msTick);
	}

    return tempSensor_readScratchPad();
}
static bool tempSensor_readScratchPad(void) // 명령갯수 -> 13
{
    if (!oneWire_isReady_forGettingCmd() ||
        oneWire_getCmdQ_reservNum() < vTempSens_cmdQInfo[TEMPSENS_CMDIDX_READ].OWcmdNum)
    {
        return false;
    }

    oneWire_cmdQueue_enQueue(OW_CMD_RESET, NULL);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_SKIPROM].cmdCode);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_READ].cmdCode);
    for (uint8_t i = 0; i < TEMPSENS_SCRATCHPAD_SIZE; i++)
    {
        oneWire_cmdQueue_enQueue(OW_CMD_READ, &vTempSensor_obj.scratchpad[i]);
    }
    oneWire_cmdQueue_enQueue(OW_CMD_RESET, NULL);

    return true;
}
static bool tempSensor_setResolution(void) // 명령갯수 -> 7
{
    uint8_t resol8Bit = (uint8_t)(~0x80);
    if (!oneWire_isReady_forGettingCmd() ||
        oneWire_getCmdQ_reservNum() < vTempSens_cmdQInfo[TEMPSENS_CMDIDX_WRITE].OWcmdNum)
    {
        return false;
    }

    oneWire_cmdQueue_enQueue(OW_CMD_RESET, NULL);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_SKIPROM].cmdCode);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_WRITE].cmdCode);

    switch (*vTempSensor_obj.currCmd.param)
    {
    case 9:
        resol8Bit &= ~(TEMPSENS_CONFIG_RESOL_R0 | TEMPSENS_CONFIG_RESOL_R1);
        break;
    case 10:
        resol8Bit |= TEMPSENS_CONFIG_RESOL_R0;
        resol8Bit &= ~TEMPSENS_CONFIG_RESOL_R1;
        break;
    case 11:
        resol8Bit &= ~TEMPSENS_CONFIG_RESOL_R0;
        resol8Bit |= TEMPSENS_CONFIG_RESOL_R1;
        break;
    case 12:
        resol8Bit |= TEMPSENS_CONFIG_RESOL_R0 | TEMPSENS_CONFIG_RESOL_R1;
        break;
    default:
        break;
    }

    vTempSensor_obj.resol8Bit = resol8Bit;

    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, &vTempSensor_obj.scratchpad[TEMPSENS_TEMP_LSB_POS]);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, &vTempSensor_obj.scratchpad[TEMPSENS_TEMP_MSB_POS]);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, &vTempSensor_obj.resol8Bit);

    oneWire_cmdQueue_enQueue(OW_CMD_RESET, NULL);

    return true;
}
static bool tempSensor_saveCurrResol(void) // 명령갯수 -> 3
{
    if (!oneWire_isReady_forGettingCmd() ||
        oneWire_getCmdQ_reservNum() < vTempSens_cmdQInfo[TEMPSENS_CMDIDX_SAVE].OWcmdNum)
    {
        return false;
    }

    oneWire_cmdQueue_enQueue(OW_CMD_RESET, NULL);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_SKIPROM].cmdCode);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_SAVE].cmdCode);

    return true;
}
static bool tempSensor_recoverPrevResol(void) // 명령갯수 -> 3
{
    if (!oneWire_isReady_forGettingCmd() ||
        oneWire_getCmdQ_reservNum() < vTempSens_cmdQInfo[TEMPSENS_CMDIDX_RECALL].OWcmdNum)
    {
        return false;
    }

    oneWire_cmdQueue_enQueue(OW_CMD_RESET, NULL);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_SKIPROM].cmdCode);
    oneWire_cmdQueue_enQueue(OW_CMD_WRITE, (uint8_t*) &vTempSens_cmdQInfo[TEMPSENS_CMDIDX_RECALL].cmdCode);

    return true;
}
static bool tempSensor_stopAllProcess(void)
{
	typStatesOW currOWstate = oneWire_getState();
    if (currOWstate == OW_STATE_ERROR || currOWstate == OW_STATE_START || currOWstate == OW_STATE_RECOVERY)
    {
        return false;
    }

    oneWire_cmdQueue_enQueue(OW_CMD_STOP, NULL);

    return true;
}

/*--------------------------------------------*/
// post Processing Functions
/*--------------------------------------------*/
static uint8_t tempSensor_CRC8(uint8_t *data, uint8_t len)
{
    uint8_t crcVal = 0, inbyte, idx, mix;

    while (len--)
    {
        inbyte = *data++;
        for (idx = 8; idx; idx--)
        {
            mix = (crcVal ^ inbyte) & 0x01;
            crcVal >>= 1;

            if (mix)
            {
                crcVal ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }

    return crcVal;
}
static void tempSensor_rawData_postProc(void) // 센서에서 9바이트를 모두 보내오면 필요한 값들을 변환해서 객체에 저장
{
    uint16_t tmp = 0;
    int8_t minus = 0, integer = 0;
    float temperVal = 0;
    uint8_t calculated_CRCVal = tempSensor_CRC8(vTempSensor_obj.scratchpad, 8); // 1. CRC 계산
    uint8_t tmpResol = 0;

    if (calculated_CRCVal != vTempSensor_obj.scratchpad[TEMPSENS_CRC_POS]) // 2. CRC 체크 -> 카운트 증가
    {
        vTempSensor_obj.lastData_IsValid_flag = false;
        vTempSensor_obj.mismatch_dataCnt++;

        if (vTempSensor_obj.mismatch_dataCnt >= TEMPSENS_DATA_INVALID_CNT_THRSLD) // 3. 연속으로 5번 미스매치시 오류로 처리
        {
            vTempSensor_obj.errCode = TEMPSENS_ERR_MALFUNC;
        }
    }
    else
    {
        vTempSensor_obj.mismatch_dataCnt = 0; // 2.1 미스매치 카운트 초기화

        vTempSensor_obj.lastData_IsValid_flag = true;

        tmpResol = vTempSensor_obj.scratchpad[TEMPSENS_RESOL_POS] & (TEMPSENS_CONFIG_RESOL_R0 | TEMPSENS_CONFIG_RESOL_R1);

        tmp = (vTempSensor_obj.scratchpad[TEMPSENS_TEMP_MSB_POS] << 8) | vTempSensor_obj.scratchpad[TEMPSENS_TEMP_LSB_POS];

        if (tmp & 0x8000)
        {
            tmp = ~tmp + 1;
            minus = 1;
        }

        integer = tmp >> 4;                  // 소수 + 정수 바이트에서 정수만 추출
        integer |= ((tmp >> 8) & 0x07) << 4; // 다음 바이트에서 부호비트를 제외한 나머지 바이트 추출

		switch (tmpResol) // 분해능에 따라 데이터시트에 적힌 비트추출해서 단위만큼 곱해서 실제온도로 변환
		{
		case 0x00:
			vTempSensor_obj.sensorResol = 9;
			temperVal = (tmp >> 3) & 0x01;
			temperVal *= vTempSens_resolInfo[0].resolution;
			break;
		case 0x20:
			vTempSensor_obj.sensorResol = 10;
			temperVal = (tmp >> 2) & 0x03;
			temperVal *= vTempSens_resolInfo[1].resolution;
			break;
		case 0x40:
			vTempSensor_obj.sensorResol = 11;
			temperVal = (tmp >> 1) & 0x07;
			temperVal *= vTempSens_resolInfo[2].resolution;
			break;
		case 0x60:
			vTempSensor_obj.sensorResol = 12;
			temperVal = tmp & 0x0F;
			temperVal *= vTempSens_resolInfo[3].resolution;
			break;
		default:
			break;
		}

        temperVal += integer; // 정수 + 실수 -> 실온도
        if (minus)
            temperVal = -temperVal;

        vTempSensor_obj.temperCelcius = temperVal;
    }
}

/*--------------------------------------------*/
// state Machine Processing Functions
/*--------------------------------------------*/
static void tempSensor_execCMD(void)
{
    switch (vTempSensor_obj.currCmd.cmd)
    {
    case TEMPSENS_CMD_WAIT:
        // do nothing
        break;

    case TEMPSENS_CMD_REQ_DATA:
        *(vTempSensor_obj.currCmd.reqFailed) = !vTempSens_cmdQInfo[TEMPSENS_CMDIDX_CONVERT].cmdFunc();
        break;

    case TEMPSENS_CMD_SET_RESOL:
        *(vTempSensor_obj.currCmd.reqFailed) = !vTempSens_cmdQInfo[TEMPSENS_CMDIDX_WRITE].cmdFunc();
        break;

    case TEMPSENS_CMD_SAVE_RESOL:
        *(vTempSensor_obj.currCmd.reqFailed) = !vTempSens_cmdQInfo[TEMPSENS_CMDIDX_SAVE].cmdFunc();
        break;

    case TEMPSENS_CMD_RECOV_PREV_RESOL:
        *(vTempSensor_obj.currCmd.reqFailed) = !vTempSens_cmdQInfo[TEMPSENS_CMDIDX_RECALL].cmdFunc();
        break;

    case TEMPSENS_CMD_STOP_ALL_PROCESS:
        *(vTempSensor_obj.currCmd.reqFailed) = !vTempSens_cmdQInfo[TEMPSENS_CMDIDX_STOP].cmdFunc();
        break;

    default:
        break;
    }
}
static typTempSens_st tempSensor_stMachine_Process(typTempSens_st prevState)
{
    typTempSens_st currState = prevState;

    if (vTempSensor_obj.OWcurrState == OW_STATE_ERROR) // 하위계층의 에러는 최우선적으로 체크할 것
    {
        currState = TEMPSENS_STATE_ERROR;

        switch (vTempSensor_obj.OWerrCode)
        {
        case OW_ERR_NOT_INITIALIZED:
            vTempSensor_obj.errCode = TEMPSENS_ERR_NOT_INITIALIZED;
            break;
        case OW_ERR_ALREADY_INITIALIZED:
            vTempSensor_obj.errCode = TEMPSENS_ERR_ALREADY_INIT;
            break;
        case OW_ERR_CANT_INITIALIZE:
            vTempSensor_obj.errCode = TEMPSENS_ERR_CANT_INITIALIZE;
            break;
        case OW_ERR_NOT_RESPONDING:
        	vTempSensor_obj.errCode = TEMPSENS_ERR_NO_RESPONSE;
            break;
        case OW_ERR_CMD_INVALID:
            // 해당 계층에서는 커맨드를 잘못 보낼 수 없는 구조 -> 또한 이 계층의 invalidCMD 코드와 목적이 다름
            break;
        case OW_ERR_NO_RESPONSE:
            // 사실 이 에러코드는 에러라기보단 warning에 가까움
            break;
        case OW_ERR_TIMEOUT:
        	vTempSensor_obj.errCode = TEMPSENS_ERR_TIMEOUT;
            break;
        default:
            break;
        }
    }
    else if (prevState != TEMPSENS_STATE_ERROR && vTempSensor_obj.OWcurrState == OW_STATE_RECOVERY) // 현재 계층이 정상상태 + 임시단선상태 체크 -> 에러전이 X
    {
        currState = TEMPSENS_STATE_BUSY;
    }
    else
    {
        switch (prevState)
        {
        case TEMPSENS_STATE_START:

            if (vTempSensor_obj.errCode == TEMPSENS_ERR_NOT_INITIALIZED || vTempSensor_obj.errCode == TEMPSENS_ERR_CANT_INITIALIZE)
            {
                currState = TEMPSENS_STATE_ERROR;
            }
            else
            {
                currState = TEMPSENS_STATE_IDLE;
            }
            break;

        case TEMPSENS_STATE_IDLE:

            if (vTempSensor_obj.errCode == TEMPSENS_ERR_ALREADY_INIT)
            {
                currState = TEMPSENS_STATE_ERROR;
            }
            else if (vTempSensor_obj.errCode == TEMPSENS_ERR_MALFUNC) // CRC 연속 미스매치
            {
                currState = TEMPSENS_STATE_ERROR;
            }
            else
            {
                if (vTempSensor_obj.currCmd.cmd != TEMPSENS_CMD_WAIT)
                {
                    currState = TEMPSENS_STATE_EXEC;
                }
            }
            break;

        case TEMPSENS_STATE_EXEC:

            if (*(vTempSensor_obj.currCmd.reqFailed) == false && vTempSensor_obj.currCmd.cmd == TEMPSENS_CMD_REQ_DATA)
            // ow로 인큐 자체가 되지 않았는데, 마냥 기다릴 수는 없는 노릇이다.
            {
                currState = TEMPSENS_STATE_PENDING;
            }
            else
            {
                currState = TEMPSENS_STATE_IDLE;
            }

            break;

        case TEMPSENS_STATE_PENDING:

            if (vTempSensor_obj.stopProc == true) // 갑자기 stop 명령이 들어옴
            {
                currState = TEMPSENS_STATE_IDLE;
            }
            else if (vTempSensor_obj.scratchpadFilled == true) // 스크래치 패드 채워짐
            {
                currState = TEMPSENS_STATE_IDLE;
            }
            else
            {
                // do nothing
            }
            break;

        case TEMPSENS_STATE_BUSY:

            if (vTempSensor_obj.OWcurrState == OW_STATE_IDLE && vTempSensor_obj.OWerrCode ==  OW_ERR_OKAY)
            {
            	currState = TEMPSENS_STATE_IDLE;
            }
            else
            {
                currState = TEMPSENS_STATE_BUSY;
            }
            break;

        case TEMPSENS_STATE_ERROR:
            // do nothing
            break;
        }
    }

    return currState;
}
static void tempSensor_stMachine_exitAction(typTempSens_st prevState)
{
    switch (prevState)
    {
    case TEMPSENS_STATE_START:
    	tempSensor_readScratchPad();
        break;

    case TEMPSENS_STATE_IDLE:
        break;

    case TEMPSENS_STATE_EXEC:
        // do nothing
        break;

    case TEMPSENS_STATE_PENDING:
        /*----조건 우선순위----*/ 

        // 1. TIMEOUT → 이번 cycle 폐기
        if (vTempSensor_obj.OWerrCode == OW_ERR_TIMEOUT)
        {
            vTempSensor_obj.scratchpadFilled = false;
            vTempSensor_obj.mismatch_dataCnt = 0;
        }
        // 2. 단선 복구상태 진입
        else if (vTempSensor_obj.OWerrCode == OW_ERR_NO_RESPONSE)
        {
            vTempSensor_obj.mismatch_dataCnt = 0;
        }
        // 3. 중단명령 수신
        else if (vTempSensor_obj.stopProc == true)
        {
            vTempSens_cmdQInfo[TEMPSENS_CMDIDX_STOP].cmdFunc();
            vTempSensor_obj.stopProc = false;
            vTempSensor_obj.scratchpadFilled = false;
            vTempSensor_obj.mismatch_dataCnt = 0;
        }
        // 4. 정상 동작 상태 -> 데이터가 들어옴 CRC 검사 필요
        else
        {
            tempSensor_rawData_postProc();
        }
        break;

    case TEMPSENS_STATE_BUSY:
        // do nothing
        break;

    case TEMPSENS_STATE_ERROR:
        // do nothing
        break;

    default:
        break;
    }
}
static void tempSensor_stMachine_entryAction(typTempSens_st currState)
{
    switch (currState)
    {
    case TEMPSENS_STATE_START:
        break;

    case TEMPSENS_STATE_IDLE:
        if (vTempSensor_obj.currCmd.cmd == TEMPSENS_CMD_REQ_DATA)
        {
            if (vTempSensor_obj.lastData_IsValid_flag == true)
            {
                vTempSensor_obj.currCmd.cmd = TEMPSENS_CMD_WAIT;
                vTempSensor_obj.currCmd.param = NULL;
                vTempSensor_obj.currCmd.reqFailed = NULL;
            }
            else
            {
                // 이전명령을 그대로 갖고감 -> 반복수행
            }
        }
        else
        {
            vTempSensor_obj.currCmd.cmd = TEMPSENS_CMD_WAIT;
            vTempSensor_obj.currCmd.param = NULL;
            vTempSensor_obj.currCmd.reqFailed = NULL;
        }
        break;

    case TEMPSENS_STATE_EXEC:
        // do nothing
        break;

    case TEMPSENS_STATE_PENDING:

        memset(vTempSensor_obj.scratchpad, TEMPSENS_PAD_INIT_VAL, 9);
        vTempSensor_obj.scratchpadFilled = false;
        break;

    case TEMPSENS_STATE_BUSY:
        // do nothing
        break;

    case TEMPSENS_STATE_ERROR:
        // do nothing
        break;

    default:
        break;
    }
}
static void tempSensor_stMachine_doAction(typTempSens_st currState)
{
    switch (currState)
    {
    case TEMPSENS_STATE_START:
        break;

    case TEMPSENS_STATE_IDLE:
        break;

    case TEMPSENS_STATE_EXEC:
        tempSensor_execCMD();
        break;

    case TEMPSENS_STATE_PENDING:

        vTempSensor_obj.scratchpadFilled = true;

        if(vTempSensor_obj.scratchpad[6] == TEMPSENS_PAD_INIT_VAL || vTempSensor_obj.scratchpad[7] == TEMPSENS_PAD_INIT_VAL)
        // 스크래치패드의 reserved 바이트넘버는 고정된 값을 가짐 -> (idx6->102) , (idx7->201)
        {
        	vTempSensor_obj.scratchpadFilled = false;
        }

        break;

    case TEMPSENS_STATE_BUSY:
        // wait
        break;

    case TEMPSENS_STATE_ERROR:
        // error hold
        break;

    default:
        break;
    }
}
static void tempSensor_stMachine(void)
{
    if (vTempSensor_obj.initialized == false)
    {
        vTempSensor_obj.errCode = TEMPSENS_ERR_NOT_INITIALIZED;
        vTempSensor_obj.currState = TEMPSENS_STATE_ERROR;
        return;
    }

    //*--------------------------*//
    // input
    //*--------------------------*//

    vTempSensor_obj.OWcurrState = oneWire_getState();
    vTempSensor_obj.OWerrCode = oneWire_getErrCode();

    //*--------------------------*//
    // state process
    //*--------------------------*//
    vTempSensor_obj.currState = tempSensor_stMachine_Process(vTempSensor_obj.prevState);

    //*--------------------------*//
    // output
    //*--------------------------*//
    if (vTempSensor_obj.prevState == vTempSensor_obj.currState)
    {
        tempSensor_stMachine_doAction(vTempSensor_obj.currState);
    }
    else
    {
        tempSensor_stMachine_exitAction(vTempSensor_obj.prevState);
        tempSensor_stMachine_entryAction(vTempSensor_obj.currState);
        tempSensor_stMachine_doAction(vTempSensor_obj.currState);

        vTempSensor_obj.prevState = vTempSensor_obj.currState;
    }

    vTempSensor_obj.stateCnt++;
}

/*--------------------------------------------*/
// user Interface Functions
/*--------------------------------------------*/
void tempSens_stMachine_init(void)
{
    if (vTempSensor_obj.initialized == true)
    {
        vTempSensor_obj.errCode = TEMPSENS_ERR_ALREADY_INIT;
        return;
    }

    if (oneWire_getErrCode() != OW_ERR_OKAY)
    {
        vTempSensor_obj.errCode = TEMPSENS_ERR_CANT_INITIALIZE;
        return;
    }

    vTempSensor_obj.scratchpad[0] = TEMPSENS_PAD_INIT_VAL;
    vTempSensor_obj.scratchpad[1] = TEMPSENS_PAD_INIT_VAL;
    vTempSensor_obj.scratchpad[2] = TEMPSENS_PAD_INIT_VAL;
    vTempSensor_obj.scratchpad[3] = TEMPSENS_PAD_INIT_VAL;
    vTempSensor_obj.scratchpad[4] = TEMPSENS_PAD_INIT_VAL;
    vTempSensor_obj.scratchpad[5] = TEMPSENS_PAD_INIT_VAL;
    vTempSensor_obj.scratchpad[6] = TEMPSENS_PAD_INIT_VAL;
    vTempSensor_obj.scratchpad[7] = TEMPSENS_PAD_INIT_VAL;
    vTempSensor_obj.scratchpad[8] = TEMPSENS_PAD_INIT_VAL;
    vTempSensor_obj.temperCelcius = 0.0;
    vTempSensor_obj.sensorResol = 0;
    vTempSensor_obj.resol8Bit = 0;
    vTempSensor_obj.scratchpadFilled = false;
    vTempSensor_obj.lastData_IsValid_flag = true;
    vTempSensor_obj.stopProc = false;

    vTempSensor_obj.errCode = TEMPSENS_ERR_OKAY;
    vTempSensor_obj.mismatch_dataCnt = 0;
    vTempSensor_obj.currState = TEMPSENS_STATE_START;
    vTempSensor_obj.prevState = TEMPSENS_STATE_START;
    vTempSensor_obj.OWcurrState = OW_STATE_START;
    vTempSensor_obj.OWerrCode = OW_ERR_OKAY;
    vTempSensor_obj.stateCnt = 0;

    vTempSensor_obj.initialized = true;
}
bool tempSensor_reqCommand(typCmd_tempSens cmd, uint8_t *param, bool *reqFailed)
{
	bool isRetryLoop = false;

    // 1. 현재 상태체크
    if (!(vTempSensor_obj.currState == TEMPSENS_STATE_IDLE ||
          vTempSensor_obj.currState == TEMPSENS_STATE_PENDING ||
          vTempSensor_obj.currState == TEMPSENS_STATE_EXEC))
    {
        return false;
    }

    // 2. 명령 번호 유효성 체크
    if (!TEMPSENS_CHK_VALID_CMD(cmd))
    {
        vTempSensor_obj.errCode = TEMPSENS_ERR_INVALID_CMD;
        return false;
    }

    // 3. SET_RESOL 파라미터 체크
    if (cmd == TEMPSENS_CMD_SET_RESOL &&
        ((param == NULL) || TEMPSENS_CHK_RESOL_RANGE(*param)))
    {
        vTempSensor_obj.errCode = TEMPSENS_ERR_INVALID_CMD;
        return false;
    }

    // 4. STOP 명령을 가장 먼저 처리 (idle이든 아니든 수락)
    if (cmd == TEMPSENS_CMD_STOP_ALL_PROCESS)
    {
        vTempSensor_obj.stopProc = true;
        vTempSensor_obj.currCmd.cmd = cmd;
        vTempSensor_obj.currCmd.param = param;
        vTempSensor_obj.currCmd.reqFailed = reqFailed;
        *(reqFailed) = false;
        return true;
    }

    // 5. IDLE 상태에서의 일반 명령 처리
    if (vTempSensor_obj.currState == TEMPSENS_STATE_IDLE)
    {
        isRetryLoop =
            (vTempSensor_obj.currCmd.cmd == TEMPSENS_CMD_REQ_DATA) &&
            (vTempSensor_obj.lastData_IsValid_flag == false) &&
            (vTempSensor_obj.mismatch_dataCnt < TEMPSENS_DATA_INVALID_CNT_THRSLD);

        if (isRetryLoop)
        {
            // CRC mismatch 반복 루프 → 새로운 명령을 받으면 안 된다
            return false;
        }

        else
        {
            vTempSensor_obj.currCmd.cmd = cmd;
            vTempSensor_obj.currCmd.param = param;
            vTempSensor_obj.currCmd.reqFailed = reqFailed;
            *(reqFailed) = false;
            return true;
        }
    }
    // 6. idle이 아니고 STOP도 아니면 reject
    else
    {
        return false;
    }
}
void tempSensor_100ms(void)
{
    // 다른 할 것
    tempSensor_stMachine();
}
float tempSensor_getTemper_celcius(void) // 이 구조는 사용자의 명령을 그냥 인큐에 넣고 나중에 찾아서 쓰는 방식이다
{
    return vTempSensor_obj.temperCelcius;
}
float tempSensor_getTemper_fahrenheit(void)
{
    return TEMPSENS_CEL2FAH(vTempSensor_obj.temperCelcius);
}
uint8_t tempSensor_getResolution(void)
{
    return vTempSensor_obj.sensorResol;
}
typTempSens_errCode tempSensor_getErrCode(void)
{
	return vTempSensor_obj.errCode;
}
bool tempSensor_isReady_forReq(void)
{
    return (vTempSensor_obj.currCmd.cmd == TEMPSENS_CMD_WAIT) && (vTempSensor_obj.currState == TEMPSENS_STATE_IDLE);
}
