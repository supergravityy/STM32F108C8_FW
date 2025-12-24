#include "OW.h"
#include <stdint.h>
#include <stdbool.h>

#define OW_LAST_CMD_NUMBER              (OW_CMD_STOP)
#define OW_1SEC_TO_500MS_TICK			(1000/5)

#pragma pack(push,1)

typedef struct oneWireCMD
{
    typCmdOW cmd;
    uint8_t* param;
}typOneWireCMD;

typedef struct oneWire
{
    // 타이머 및 gpio 객체 포인터
    GPIO_TypeDef* GPIOx;
    uint16_t DIO_Pin;
    TIM_HandleTypeDef* htimX;

    // 상태 표현 멤버목록
    bool initialized;               // 객체 초기화 여부
    typErrCodeOW errCode;           // 에러발생시 에러 확인용
    typStatesOW currState;          // 상태머신의 현재 상태
    typStatesOW prevState;          // 상태머신의 이전 상태
    uint32_t stateCnt;              // 상태머신의 총합 tick 횟수

    // 오류제어용 멤버변수 목록
    uint16_t rcvrFailCount;         // 임시 단선상태에서의 리커버리
    uint8_t rdyChk_Cnt_5ms;         // 센서 준비 상태 시간까지 경과한 5ms 단위 시간 카운터
    uint8_t rdyChk_TO_5ms;          // 센서 준비 상태 타임아웃 설정값 (5ms 단위)
    bool rdyChk_cmpltFlag;          // 센서 준비 완료 플래그
    bool rdyChk_timeOut_flag;       // 센서 준비시간 초과 플래그

    // 명령 큐 관련 멤버목록
    bool stopFlag;
    typOneWireCMD cmdQ[OW_CMDQ_SIZE];
    typOneWireCMD cmdNext;
    uint8_t cmdNum;
}typOneWire;

#pragma pack(pop)

static typOneWire vOneWire_obj;

/*----------------------------------------*/
// OneWire GPIO control Functions
/*----------------------------------------*/
inline static void oneWire_delay_us(uint16_t time_us)
{
	OW_DISABLE_IRQ();
    vOneWire_obj.htimX->Instance->CNT = 0;
    while (vOneWire_obj.htimX->Instance->CNT <= time_us);
    OW_ENABLE_IRQ();
}
// BSRR 레지스터는 각 비트의 상태에 따라 독립적으로 set/reset 동작을 수행가능
inline static void oneWire_setLow(GPIO_TypeDef* GPIOx, uint16_t DIO_Pin)
{
    // 16~31 bit (1) -> 핀을 low 상태로 만듦
    GPIOx->BSRR = DIO_Pin << 16;
}
inline static void oneWire_setHigh(GPIO_TypeDef* GPIOx, uint16_t DIO_Pin)
{
    // 0~15 bit (1) -> 핀을 high 상태로 만듦
    GPIOx->BSRR = DIO_Pin;
}
// GPIO 모드를 입/출력으로 설정
inline static void oneWire_setInput(typOneWire* oneWire_obj)
{
    GPIO_InitTypeDef gpioInit;
    gpioInit.Mode = GPIO_MODE_INPUT;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    gpioInit.Pin = oneWire_obj->DIO_Pin;
    HAL_GPIO_Init(oneWire_obj->GPIOx, &gpioInit);
}
inline static void oneWire_setOutput(typOneWire* oneWire_obj)
{
    GPIO_InitTypeDef gpioInit;
    gpioInit.Mode = GPIO_MODE_OUTPUT_OD;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    gpioInit.Pin = oneWire_obj->DIO_Pin;
    HAL_GPIO_Init(oneWire_obj->GPIOx, &gpioInit);
}
static void oneWire_busInit(void)
{
    oneWire_setOutput(&vOneWire_obj);
    oneWire_setHigh(vOneWire_obj.GPIOx, vOneWire_obj.DIO_Pin);
    oneWire_delay_us(1000);
    oneWire_setLow(vOneWire_obj.GPIOx, vOneWire_obj.DIO_Pin);
    oneWire_delay_us(1000);
    oneWire_setHigh(vOneWire_obj.GPIOx, vOneWire_obj.DIO_Pin);
    oneWire_delay_us(2000);
}
// 타임 슬롯 시작(15us 동안 LOW가 start트리거), 15us 근방(총 30us부근)에서 센서는 데이터를 샘플링함
static inline void oneWire_writeBit(typOneWire* oneWire_obj, uint8_t bit)
{
    if(bit == 0x01) // 1. 1비트 출력 -> 70us 시간필요
    {
        oneWire_setLow(oneWire_obj->GPIOx, oneWire_obj->DIO_Pin);
        oneWire_setOutput(oneWire_obj);
        oneWire_delay_us(10);                   // 1.1 10us 동안 LOW 유지

        oneWire_setInput(oneWire_obj);        // 1.2 라인 해제 -> 풀업저항때문에 HIGH 상태됨

        oneWire_delay_us(55);                   // 1.3 60us 대기 -> 센서가 샘플링을 하는 시간 고려
        oneWire_setInput(oneWire_obj);
    }
    else            // 2. 0비트 출력 -> 70us 시간필요
    {
        oneWire_setLow(oneWire_obj->GPIOx, oneWire_obj->DIO_Pin);
        oneWire_setOutput(oneWire_obj);
        oneWire_delay_us(65);                   // 2.1 샘플링 전체시간동안 LOW 유지

        oneWire_setInput(oneWire_obj);        // 2.2 라인 해제 -> 풀업저항때문에 HIGH 상태됨

        oneWire_delay_us(5);                    // 2.3 5us 대기 -> 그냥 마진용
        oneWire_setInput(oneWire_obj);
    }
}
static inline uint8_t oneWire_readBit(typOneWire* oneWire_obj)
{
    uint8_t retBit = 0x0;

    oneWire_setLow(oneWire_obj->GPIOx, oneWire_obj->DIO_Pin);
    oneWire_setOutput(oneWire_obj);
    oneWire_delay_us(2);                        // 1. 라인을 LOW 상태로 2us 유지 (1us 이상)

    oneWire_setInput(oneWire_obj);            // 2. 라인 해제 -> 풀업저항때문에 HIGH 상태됨 (high impedance 상태)
    oneWire_delay_us(10);                       // 3. 10us 대기 후 샘플링 시작

    if(HAL_GPIO_ReadPin(oneWire_obj->GPIOx, oneWire_obj->DIO_Pin) == GPIO_PIN_SET) 
    {
        retBit = 0x1;
    }

    oneWire_delay_us(50);                       // 4. 나머지 시간 대기 -> 총 60us 타임슬롯 유지

    return retBit;
    // 총 62us 소요
}

/*----------------------------------------*/
// OneWire command Functions
/*----------------------------------------*/
static void oneWire_writeByte(typOneWire* oneWire_obj, uint8_t byte)
{
    uint8_t pos = 0x8;

    while(pos--) // 8 * 70us = 560us
    {
    	oneWire_writeBit(oneWire_obj, byte & 0x01);
    	byte >>= 1;
    }
}
static uint8_t oneWire_readByte(typOneWire* oneWire_obj)
{
    uint8_t byte = 0x0, pos = 0x8;

    while(pos--) // 8 * 62us = 496us
    {
    	byte >>= 1;
    	byte |= (oneWire_readBit(oneWire_obj) << 7);
    }

    return byte;
}
static void oneWire_readyCheck(typOneWire* oneWire_obj)
{
	if(oneWire_obj->rdyChk_TO_5ms == OW_1SEC_TO_500MS_TICK) // 첫 read이기에 대기시간을 모름 -> 1초동안 계속 읽기
	{
		if (oneWire_obj->rdyChk_Cnt_5ms < oneWire_obj->rdyChk_TO_5ms + OW_RDYCHK_EXTRA_5msTick)
		{
			oneWire_obj->rdyChk_cmpltFlag = oneWire_readBit(oneWire_obj);
			if (oneWire_obj->rdyChk_cmpltFlag == true) // 준비완료 신호 감지됨
			{
				return;
			}

			oneWire_obj->rdyChk_Cnt_5ms++;
		}
		else // 시간초과상태
		{
			oneWire_obj->rdyChk_timeOut_flag = true;
			oneWire_obj->errCode = OW_ERR_TIMEOUT;
			return;
		}
	}
	else // 상위계층으로부터 대기시간을 전달받음 -> 첫 read가 아님
	{
		if (oneWire_obj->rdyChk_Cnt_5ms < oneWire_obj->rdyChk_TO_5ms + OW_RDYCHK_EXTRA_5msTick) // 초과여유분 시간까지 대기
		{
			if (oneWire_obj->rdyChk_TO_5ms <= oneWire_obj->rdyChk_Cnt_5ms) // 데이터시트에서 기술한 시간이후부터 여유분시간까지 비트하나읽기
			{
				oneWire_obj->rdyChk_cmpltFlag = oneWire_readBit(oneWire_obj);
				if (oneWire_obj->rdyChk_cmpltFlag == true) // 준비완료 신호 감지됨
				{
					return;
				}
			}

			oneWire_obj->rdyChk_Cnt_5ms++;
		}
		else // 시간초과상태
		{
			oneWire_obj->rdyChk_timeOut_flag = true;
			oneWire_obj->errCode = OW_ERR_TIMEOUT;
			return;
		}
	}
}
static void oneWire_reset(typOneWire* oneWire_obj)
{
    GPIO_PinState presence;

    // 버스라인을 Low화 -> 480us 동안 + 여유 20us
    // -> 온도센서에 리셋신호 전송
    oneWire_setLow(oneWire_obj->GPIOx, oneWire_obj->DIO_Pin);
    oneWire_setOutput(oneWire_obj);
    oneWire_delay_us(480);
    oneWire_delay_us(20);

    // 온도센서의 피드백신호 대기 -> 70us 동안
    oneWire_setHigh(oneWire_obj->GPIOx, oneWire_obj->DIO_Pin);
    oneWire_setInput(oneWire_obj);
    oneWire_delay_us(70);

    // 온도센서의 피드백 신호 읽기
    // 0 -> 장치 존재, 1 -> 장치 없음
    presence = HAL_GPIO_ReadPin(oneWire_obj->GPIOx, oneWire_obj->DIO_Pin);

    if(presence == GPIO_PIN_SET) 
    {
        oneWire_obj->errCode = OW_ERR_NO_RESPONSE;    // 단선감지
    }
    else
    {
        oneWire_obj->errCode = OW_ERR_OKAY;         // 장치가 존재함
    }

    // 버스가 안정화 되는시간 -> 스케줄링 형태라서 자동으로 시간을 가져감
    // oneWire_delay_us(410);
}
static void oneWire_recoveryBus(typOneWire *oneWire_obj)
{
    // reset 과정 중, presence pulse 대기시간 동안 버스가 high 상태로 고정되어 있는지 확인
    // 만약 고정되어있다면, 온도센서가 gnd를 인가하지 못함 -> 풀업저항이 걸려버림 -> 온도센서 이상

    oneWire_reset(oneWire_obj);

    if(oneWire_obj->errCode == OW_ERR_NO_RESPONSE)
    {
        oneWire_obj->rcvrFailCount++;
    }
    else
    {
        oneWire_obj->errCode = OW_ERR_OKAY;
    }

    if(oneWire_obj->rcvrFailCount >= OW_NO_RESPOND_THRESHOLD_5msTick) // 1sec 동안 단선됨
    {
        oneWire_obj->errCode = OW_ERR_NOT_RESPONDING; // 단선확인
    }
}

/*----------------------------------------*/
// Command Queue Functions
/*----------------------------------------*/
static void oneWire_cmdQueue_flush(void)
{
    for(uint8_t i = 0; i < OW_CMDQ_SIZE; i++)
    {
        vOneWire_obj.cmdQ[i].cmd = OW_CMD_IDLE;
        vOneWire_obj.cmdQ[i].param = NULL;
    }
    vOneWire_obj.cmdNext.cmd = OW_CMD_IDLE;
    vOneWire_obj.cmdNext.param = NULL;
    vOneWire_obj.cmdNum = 0;
}
static typOneWireCMD oneWire_cmdQueue_deQueue(void) // 맨 앞의 명령어를 꺼내고, 큐를 앞으로 당김
{
    typOneWireCMD upperCMD = {OW_CMD_IDLE, 0};

    if (vOneWire_obj.cmdNum == 0)   // 큐가 비어있음
    {
        return upperCMD;
    }
    else if(vOneWire_obj.prevState == OW_STATE_ERROR) // 에러상태에서는 큐를 건드리지 않음
    {
        return upperCMD;
    }
    else if(vOneWire_obj.errCode == OW_ERR_NO_RESPONSE) // 임시 단선상태
    {
        // 이때에는 큐를 건드리지 않음
        return upperCMD;
    }
    
    else
    {
        upperCMD = vOneWire_obj.cmdQ[0]; // 큐에서 하나 꺼내기

        if(upperCMD.cmd > OW_LAST_CMD_NUMBER) // 유효하지 않은 명령어인 경우
        {
            upperCMD.cmd = OW_CMD_IDLE;
            upperCMD.param = 0;
            vOneWire_obj.errCode = OW_ERR_CMD_INVALID;
        }
        
        for (uint8_t i = 0; i < OW_CMDQ_SIZE - 1; i++) // 앞으로 한 칸씩 당기기
        {
            if (i < vOneWire_obj.cmdNum - 1) // 마지막 명령어 이후는 당기지 않음
                vOneWire_obj.cmdQ[i] = vOneWire_obj.cmdQ[i + 1];
            else
            {
                vOneWire_obj.cmdQ[i].cmd = OW_CMD_IDLE;
                vOneWire_obj.cmdQ[i].param = 0;
            }
        }

        vOneWire_obj.cmdNum--; // 큐에 남은 명령어 수 감소
    }

    return upperCMD;
}
static typStatesOW oneWire_cmdQueue_fetchCMD(void)
{
    switch (vOneWire_obj.cmdNext.cmd)
    {
        case OW_CMD_IDLE:
            return OW_STATE_IDLE;
        break;

        case OW_CMD_RESET:
            return OW_STATE_RESET;
        break;

        case OW_CMD_WRITE:
            return OW_STATE_WRITE;
        break;

        case OW_CMD_READ:
            return OW_STATE_READ;
        break;

        case OW_CMD_RDY_CHK:
            return OW_STATE_RDY_CHK;
        break;

        case OW_CMD_STOP: // 현재 작업을 모두 취소하고 큐를 비움
        	oneWire_cmdQueue_flush();
        	vOneWire_obj.stopFlag = false;
            return OW_STATE_IDLE;
        break;
        
        default:
            vOneWire_obj.errCode = OW_ERR_CMD_INVALID;
        return OW_STATE_ERROR;
    }
}
void oneWire_cmdQueue_enQueue(typCmdOW cmd,  uint8_t* param) // 큐의 맨 뒤에 명령어 추가
{
    if(vOneWire_obj.currState == OW_STATE_ERROR)
        return;

    if (vOneWire_obj.cmdNum >= OW_CMDQ_SIZE) // 큐가 가득 참
    {
        return;
    }
    else
    {
    	if(cmd == OW_CMD_STOP)
    	{
    		vOneWire_obj.stopFlag = true;
    		return;
    	}

        vOneWire_obj.cmdQ[vOneWire_obj.cmdNum].cmd = cmd; // 큐의 맨 뒤에 명령어 추가
        vOneWire_obj.cmdQ[vOneWire_obj.cmdNum].param = param;
        
        vOneWire_obj.cmdNum++; // 큐에 남은 명령어 수 증가
        return;
    }
}

/*----------------------------------------*/
// state machine Functions
/*----------------------------------------*/
static typStatesOW oneWire_stMachine_Process(typStatesOW prevState, uint8_t *errCode)
{
    typStatesOW currState = prevState;

    switch (prevState)
    {
    case OW_STATE_START:

        if (vOneWire_obj.errCode == OW_ERR_NOT_INITIALIZED)
            currState = OW_STATE_ERROR;
        else if (vOneWire_obj.errCode == OW_ERR_CANT_INITIALIZE)
            currState = OW_STATE_ERROR;
        else
            currState = OW_STATE_IDLE;

        break;

    case OW_STATE_IDLE:

        if(vOneWire_obj.errCode == OW_ERR_ALREADY_INITIALIZED)
            currState = OW_STATE_ERROR;
        else
            currState = oneWire_cmdQueue_fetchCMD();

        break;

    case OW_STATE_RESET:
        if(vOneWire_obj.errCode == OW_ERR_NO_RESPONSE)
            currState = OW_STATE_RECOVERY;
        else
            currState = OW_STATE_IDLE;
        break;

    case OW_STATE_WRITE:
        currState = OW_STATE_IDLE;
        break;

    case OW_STATE_READ:
        currState = OW_STATE_IDLE;
        break;

    case OW_STATE_RDY_CHK:

    	if(vOneWire_obj.stopFlag == true)
    	{
    		currState = OW_STATE_IDLE; // flush 처리는 idle entryAction에서
    	}
    	else if(vOneWire_obj.rdyChk_cmpltFlag == true)
        {
            currState = OW_STATE_IDLE;
        }
        else
        {
            if(vOneWire_obj.rdyChk_timeOut_flag == true)
                currState = OW_STATE_ERROR;
            else
                currState = OW_STATE_RDY_CHK;
        }
        break;

    case OW_STATE_RECOVERY:

    if(vOneWire_obj.errCode == OW_ERR_NO_RESPONSE)
        currState = prevState;

    else if(vOneWire_obj.errCode == OW_ERR_NOT_RESPONDING)
        currState = OW_STATE_ERROR;

    else if(vOneWire_obj.errCode == OW_ERR_OKAY)
        currState = OW_STATE_IDLE;
    else{}
        break;

    case OW_STATE_ERROR:
    // do nothing
        break;

    default:

        *errCode = OW_ERR_CMD_INVALID;
        currState = OW_STATE_ERROR;

        break;
    }

    return currState;
}
static void oneWire_stMachine_doAction(typStatesOW currState)
{
    switch (currState)
    {
    case OW_STATE_START:
    // do notihing
        break;

    case OW_STATE_IDLE:
    // do nothing
        break;

    case OW_STATE_RESET:
        oneWire_reset(&vOneWire_obj);
        break;

    case OW_STATE_WRITE:
        oneWire_writeByte(&vOneWire_obj, *(vOneWire_obj.cmdNext.param));
        break;

    case OW_STATE_READ:
        *(vOneWire_obj.cmdNext.param) = oneWire_readByte(&vOneWire_obj);
        break;

    case OW_STATE_RDY_CHK:
        oneWire_readyCheck(&vOneWire_obj);
        break;

    case OW_STATE_RECOVERY:

        oneWire_cmdQueue_flush();           // 임시 단선상황에선 reset으로 복귀하기 때문에 큐를 모두 비워야 함
        oneWire_recoveryBus(&vOneWire_obj); // 그리고 1초동안 -> 200번의 reset 시도
        break;

    case OW_STATE_ERROR:
    // do nothing
        break;

    default:
        break;
    }
}
static void oneWire_stMachine_exitAction(typStatesOW prevState)
{
    switch (prevState)
    {
    case OW_STATE_START:
        break;

    case OW_STATE_IDLE:
        break;

    case OW_STATE_RESET:
        vOneWire_obj.cmdNext.cmd = OW_CMD_IDLE;
        vOneWire_obj.cmdNext.param = NULL;
        break;

    case OW_STATE_WRITE:
        vOneWire_obj.cmdNext.cmd = OW_CMD_IDLE;
        vOneWire_obj.cmdNext.param = NULL;
        break;

    case OW_STATE_READ:
        vOneWire_obj.cmdNext.cmd = OW_CMD_IDLE;
        vOneWire_obj.cmdNext.param = NULL;
        break;

    case OW_STATE_RDY_CHK:
        vOneWire_obj.cmdNext.cmd = OW_CMD_IDLE;
        vOneWire_obj.cmdNext.param = NULL;
        break;

    case OW_STATE_RECOVERY:
        // do nothing
        break;

    case OW_STATE_ERROR:
        // do nothing
        break;

    default:
        break;
    }
}
static void oneWire_stMachine_entryAction(typStatesOW currState)
{
    switch (currState)
    {
    case OW_STATE_START:
        break;

    case OW_STATE_IDLE:
    	if(vOneWire_obj.stopFlag == true)
    	{
    		oneWire_cmdQueue_flush();
    		vOneWire_obj.stopFlag = false;
    	}
        break;

    case OW_STATE_RESET:
        break;

    case OW_STATE_WRITE:
        break;

    case OW_STATE_READ:
        break;

    case OW_STATE_RDY_CHK:

        vOneWire_obj.rdyChk_Cnt_5ms = 0;

		if (vOneWire_obj.cmdNext.param == NULL) // 처음이라 센서에 얼마의 분해능이 저장되었는지 모름
		{
			vOneWire_obj.rdyChk_TO_5ms = OW_1SEC_TO_500MS_TICK;
		}
		else // 분해능이 얼마인지 알고있음 -> 계산가능
		{
			vOneWire_obj.rdyChk_TO_5ms = *(vOneWire_obj.cmdNext.param);
		}

        vOneWire_obj.rdyChk_timeOut_flag = false;
        vOneWire_obj.rdyChk_cmpltFlag = false;
        break;

    case OW_STATE_RECOVERY:
        vOneWire_obj.rcvrFailCount = 0;
        break;

    case OW_STATE_ERROR:
    // do nothing
        break;

    default:
        break;
    }
}
static void oneWire_stMachine(void)
{
    if (vOneWire_obj.initialized == false)
    {
        vOneWire_obj.errCode = OW_ERR_NOT_INITIALIZED;
        vOneWire_obj.currState = OW_STATE_ERROR;
        return;
    }

    //*--------------------------*//
    // input
    //*--------------------------*//
    if (vOneWire_obj.currState == OW_STATE_IDLE)
    {
        vOneWire_obj.cmdNext = oneWire_cmdQueue_deQueue();
    }
    else
    {
        // 현재 상태가 IDLE이 아니면 커맨드 큐에서 명령을 가져오지 않음
    }

    //*--------------------------*//
    // state process
    //*--------------------------*//
    vOneWire_obj.currState = oneWire_stMachine_Process(vOneWire_obj.prevState, (uint8_t *)&vOneWire_obj.errCode);

    //*--------------------------*//
    // output
    //*--------------------------*//
    if (vOneWire_obj.prevState == vOneWire_obj.currState)
    {
        oneWire_stMachine_doAction(vOneWire_obj.currState);
    }
    else
    {
        oneWire_stMachine_exitAction(vOneWire_obj.prevState);
        oneWire_stMachine_entryAction(vOneWire_obj.currState);
        oneWire_stMachine_doAction(vOneWire_obj.currState);

        vOneWire_obj.prevState = vOneWire_obj.currState;
    }

    vOneWire_obj.stateCnt++;
}

/*----------------------------------------*/
// user interface functions
/*----------------------------------------*/
void oneWire_stMachine_init(GPIO_TypeDef *GPIOx, uint16_t DIO_Pin, TIM_HandleTypeDef *htimX)
{
    if (GPIOx == NULL || htimX == NULL || DIO_Pin == 0)
    {
        vOneWire_obj.errCode = OW_ERR_CANT_INITIALIZE;
    }
    else if (vOneWire_obj.initialized == true)
    {
        vOneWire_obj.errCode = OW_ERR_ALREADY_INITIALIZED;
    }
    else
    {
        vOneWire_obj.GPIOx = GPIOx;
        vOneWire_obj.DIO_Pin = DIO_Pin;
        vOneWire_obj.htimX = htimX;
        HAL_TIM_Base_Start(vOneWire_obj.htimX);
        oneWire_busInit();

        vOneWire_obj.errCode = OW_ERR_OKAY;
        vOneWire_obj.currState = OW_STATE_START;
        vOneWire_obj.prevState = OW_STATE_START;

        vOneWire_obj.rdyChk_Cnt_5ms = 0;
        vOneWire_obj.rdyChk_TO_5ms = 0;
        vOneWire_obj.rdyChk_cmpltFlag = false;
        vOneWire_obj.rdyChk_timeOut_flag = false;
        vOneWire_obj.stopFlag = false;

        oneWire_cmdQueue_flush();

        vOneWire_obj.rcvrFailCount = 0;

        vOneWire_obj.initialized = true;
    }
}
void oneWire_process_5ms(void)
{
    // 다른 할 것
    oneWire_stMachine();
}
bool oneWire_isReady_forGettingCmd(void) // 상위계층에서 enqueue를 할 준비가 되었는지를 확인가능
{
    if(vOneWire_obj.currState == OW_STATE_RECOVERY) // 리커버리는 1초동안 polling -> 오래걸림
    {
        return false;
    }
    else if(vOneWire_obj.currState == OW_STATE_RDY_CHK) // 분해능 12bit 시, 760ms 까지의 시간이 필요함 -> 오래걸림
    {
        return false;
    }
    else if(vOneWire_obj.currState == OW_STATE_START) // 아직 초기화도 안된 상태 -> 인큐 불가능
    {
        return false;
    }
    else if(vOneWire_obj.currState == OW_STATE_ERROR) // 에러상태면 MCU 초기화 필요 -> 인큐 불가능
    {
        return false;
    }
    else
    {
        return true;
    }
}
typErrCodeOW oneWire_getErrCode(void)
{
    return vOneWire_obj.errCode;
}
uint8_t oneWire_getCmdQ_reservNum(void)
{
    return OW_CMDQ_SIZE - vOneWire_obj.cmdNum;
}
typStatesOW oneWire_getState(void)
{
    return vOneWire_obj.currState;
}