/**********************************************************************************************************************
 * STM + FSM Traffic Light Example
 *********************************************************************************************************************/

#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxStm.h"
#include "IfxCpu_Irq.h"
#include "IfxPort.h"
#include "IfxPort_PinMap.h"

/*----------------------------------------------------*/
/* LED 핀 정의 (보드에 맞게 수정 가능) */
/* P10.1 = RED LED */
/* P10.2 = BLUE LED */
/*----------------------------------------------------*/

#define RED_LED     IfxPort_P10_1
#define BLUE_LED    IfxPort_P10_2

#define STM_INTERRUPT_PRIORITY   100
#define STM_TICK_500MS           5000000u   // 100MHz 기준 0.05초

#define BTN_INTERRUPT_PRIORITY   110

// ERU (External Request Unit) for S3
#define EXIS0_IDX 4
#define FEN0_IDX 8
#define REN0_IDX 9
#define EIEN0_IDX 11
#define INP0_IDX 12
#define IGP0_IDX 14

// SRC (Service Request Control) for S3
#define SRE_IDX 10
#define TOS_IDX 11

IfxCpu_syncEvent cpuSyncEvent = 0;

/*----------------------------------------------------*/
/* STM 구조체 */
/*----------------------------------------------------*/
typedef struct
{
    Ifx_STM *stmSfr;
    IfxStm_CompareConfig stmConfig;
} App_Stm;

App_Stm g_Stm;

/*----------------------------------------------------*/
/* FSM 상태 정의 */
/*----------------------------------------------------*/
typedef enum
{
    STATE_RED = 0,
    STATE_BLUE,
    STATE_BLUE_BLINK
} TrafficState;

volatile TrafficState g_state = STATE_RED;
volatile uint32 g_tickCount = 0;
volatile uint8 g_blinkFlag = 0;

volatile uint8 g_pause = 0;   // 0: 동작중, 1: 일시정지

/*----------------------------------------------------*/
/* 인터럽트 선언 */
/*----------------------------------------------------*/
IFX_INTERRUPT(STM_Int0Handler, 0, STM_INTERRUPT_PRIORITY);
IFX_INTERRUPT(BTN_ISR0, 0, BTN_INTERRUPT_PRIORITY);


void BTN_ISR0(void)
{
    if(g_state == STATE_BLUE_BLINK) return; // 파란 깜빡 상태에서는 무시
    g_pause ^= 1;
}

/*----------------------------------------------------*/
/* GPIO 초기화 */
/*----------------------------------------------------*/
void initGPIO(void)
{
    IfxPort_setPinModeOutput(RED_LED.port, RED_LED.pinIndex,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

    IfxPort_setPinModeOutput(BLUE_LED.port, BLUE_LED.pinIndex,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

    IfxPort_setPinLow(RED_LED.port, RED_LED.pinIndex);
    IfxPort_setPinLow(BLUE_LED.port, BLUE_LED.pinIndex);
}

/*----------------------------------------------------*/
/* STM 초기화 */
/*----------------------------------------------------*/
void initSTM(void)
{
    boolean interruptState = IfxCpu_disableInterrupts();

    g_Stm.stmSfr = &MODULE_STM0;

    IfxStm_initCompareConfig(&g_Stm.stmConfig);

    g_Stm.stmConfig.triggerPriority = STM_INTERRUPT_PRIORITY;
    g_Stm.stmConfig.typeOfService   = IfxSrc_Tos_cpu0;
    g_Stm.stmConfig.ticks           = STM_TICK_500MS;

    IfxStm_initCompare(g_Stm.stmSfr, &g_Stm.stmConfig);

    IfxCpu_restoreInterrupts(interruptState);
}

/*----------------------------------------------------*/
/* STM 인터럽트 (FSM 핵심) */
/*----------------------------------------------------*/
void STM_Int0Handler(void)
{
    /* 인터럽트 플래그 클리어 */
    IfxStm_clearCompareFlag(g_Stm.stmSfr, g_Stm.stmConfig.comparator);

    /* 다음 0.05초 예약 */
    IfxStm_increaseCompare(g_Stm.stmSfr,
                           g_Stm.stmConfig.comparator,
                           STM_TICK_500MS);

    g_tickCount++;   // 0.05초마다 증가

    switch(g_state)
    {
        case STATE_RED:

            IfxPort_setPinHigh(RED_LED.port, RED_LED.pinIndex);
            IfxPort_setPinLow(BLUE_LED.port, BLUE_LED.pinIndex);

            if(!g_pause)
            {
                if(g_tickCount >= 10)   // 0.5초
                {
                    g_tickCount = 0;
                    g_state = STATE_BLUE;
                }
            }
            break;


        case STATE_BLUE:

            IfxPort_setPinLow(RED_LED.port, RED_LED.pinIndex);
            IfxPort_setPinHigh(BLUE_LED.port, BLUE_LED.pinIndex);

            if(!g_pause){
                if(g_tickCount >= 10)
                {
                    g_tickCount = 0;
                    g_state = STATE_BLUE_BLINK;
                }
            }
            break;


        case STATE_BLUE_BLINK:

            IfxPort_setPinLow(RED_LED.port, RED_LED.pinIndex);

            /* 0.05초마다 토글 */
            if(g_blinkFlag == 0)
            {
                IfxPort_setPinHigh(BLUE_LED.port, BLUE_LED.pinIndex);
                g_blinkFlag = 1;
            }
            else
            {
                IfxPort_setPinLow(BLUE_LED.port, BLUE_LED.pinIndex);
                g_blinkFlag = 0;
            }

            if(g_tickCount >= 10)
            {
                g_tickCount = 0;
                g_state = STATE_RED;
            }
            break;
    }
}

void initERU(void)
{
    // ERU (External Request Unit) for S3
    SCU_EICR1.U &= ~(0x7 << EXIS0_IDX);
    SCU_EICR1.U |= 0x1 << EXIS0_IDX;

    SCU_EICR1.U &= ~(0x1 << REN0_IDX);
    SCU_EICR1.U |= 0x1 << FEN0_IDX;

    SCU_EICR1.U |= 0x1 << EIEN0_IDX;

    SCU_EICR1.U &= ~(0x7 << INP0_IDX);

    SCU_IGCR0.U &= ~(0x3 << IGP0_IDX);
    SCU_IGCR0.U |= 0x1 << IGP0_IDX;

    // SRC (Service Request Control) for S3
    SRC_SCU_SCU_ERU0.U &= ~0xFF;
    SRC_SCU_SCU_ERU0.U |= 110;

    SRC_SCU_SCU_ERU0.U |= 0x1 << SRE_IDX;
    SRC_SCU_SCU_ERU0.U &= ~(0x3 << TOS_IDX);
}

/*----------------------------------------------------*/
/* 메인 */
/*----------------------------------------------------*/
void core0_main(void)
{
    IfxCpu_enableInterrupts();

    /* Watchdog Disable */
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    /* CPU Sync */
    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    initGPIO();
    initSTM();
    initERU();

    while(1)
    {
        /* 모든 동작은 인터럽트에서 수행 */
    }
}
