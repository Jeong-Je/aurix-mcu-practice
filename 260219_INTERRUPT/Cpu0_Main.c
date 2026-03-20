/**********************************************************************************************************************
 * 버튼(S3) 인터럽트로 LED 깜빡임을 On/Off 제어하는 예제
 * - ERU를 이용해 외부 인터럽트 설정
 * - ISR에서는 플래그만 세우고, 실제 처리는 main loop에서 수행
 * - Timer 없이 for문으로 간단한 delay 구현
 *********************************************************************************************************************/

#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"

// 핀 관련 비트 위치
#define PCn_2_IDX 19
#define P2_IDX 2
#define PCn_1_IDX 11
#define P1_IDX 1

// ERU 설정용 비트
#define EXIS0_IDX 4
#define FEN0_IDX 8
#define REN0_IDX 9
#define EIEN0_IDX 11
#define INP0_IDX 12
#define IGP0_IDX 14

// SRC 설정용 비트
#define SRE_IDX 10
#define TOS_IDX 11

IfxCpu_syncEvent cpuSyncEvent = 0;

// ISR → main으로 전달할 플래그
volatile uint8 buttonRequest = 0;

// LED 동작 상태
typedef enum {
    STATE_BLINKING,
    STATE_IDLE
} systemState;

systemState currentState = STATE_BLINKING;

// SRPN 0x10 인터럽트 발생 시 ISR0 실행
IFX_INTERRUPT(ISR0, 0, 0x10);
void ISR0(void)
{
    // ISR에서는 최대한 가볍게
    buttonRequest = 1;
}

int core0_main(void)
{
    IfxCpu_enableInterrupts();

    // 실습이라 watchdog 끔
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    // 멀티코어 동기화
    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    initGPIO();
    initLED();
    initERU();

    while(1)
    {
        // 버튼 눌리면 상태 토글
        if (buttonRequest)
        {
            currentState = (currentState == STATE_BLINKING) ? STATE_IDLE : STATE_BLINKING;
            buttonRequest = 0;
        }

        switch (currentState)
        {
            case STATE_BLINKING:
                blinking();
                // 간단한 딜레이 (busy-wait)
                for(volatile int i=0; i<10000000; i++);
                break;

            case STATE_IDLE:
                // 아무것도 안함 (LED 멈춤)
                break;

            default:
                currentState = STATE_IDLE;
                buttonRequest = 0;
                break;
        }
    }

    return (1);
}

void initLED(void)
{
    // 초기 상태: P10.2 OFF, P10.1 ON
    P10_OMR.U = (1 << (P2_IDX + 16));
    P10_OMR.U = (1 << P1_IDX);
}

void initGPIO(void)
{
    // S3 버튼 (P02.1) 입력 설정
    P02_IOCR0.U &= ~(0x1F << PCn_1_IDX);
    P02_IOCR0.U |= 0x02 << PCn_1_IDX;

    // LED (P10.2) 출력 설정
    P10_IOCR0.U &= ~(0x1F << PCn_2_IDX);
    P10_IOCR0.U |= 0x10 << PCn_2_IDX;

    // LED (P10.1) 출력 설정
    P10_IOCR0.U &= ~(0x1F << PCn_1_IDX);
    P10_IOCR0.U |= 0x10 << PCn_1_IDX;
}

void initERU(void)
{
    // P02.1을 ERU 입력으로 연결
    SCU_EICR1.U &= ~(0x7 << EXIS0_IDX);
    SCU_EICR1.U |= 0x1 << EXIS0_IDX;

    // Falling edge에서 인터럽트 발생 (버튼 눌림)
    SCU_EICR1.U &= ~(0x1 << REN0_IDX);
    SCU_EICR1.U |= 0x1 << FEN0_IDX;

    // 인터럽트 활성화
    SCU_EICR1.U |= 0x1 << EIEN0_IDX;

    SCU_EICR1.U &= ~(0x7 << INP0_IDX);

    // 게이팅 설정
    SCU_IGCR0.U &= ~(0x3 << IGP0_IDX);
    SCU_IGCR0.U |= 0x1 << IGP0_IDX;

    // SRPN = 0x10 → ISR0와 연결
    SRC_SCU_SCU_ERU0.U &= ~0xFF;
    SRC_SCU_SCU_ERU0.U |= 0x10;

    SRC_SCU_SCU_ERU0.U |= 0x1 << SRE_IDX;   // 인터럽트 enable
    SRC_SCU_SCU_ERU0.U &= ~(0x3 << TOS_IDX); // CPU0에서 처리
}

void blinking(void)
{
    // P10.2 토글
    if (P10_OUT.U & (1 << P2_IDX))
        P10_OMR.U = (1 << (P2_IDX + 16));
    else
        P10_OMR.U = (1 << P2_IDX);

    // P10.1 토글
    if (P10_OUT.U & (1 << P1_IDX))
        P10_OMR.U = (1 << (P1_IDX + 16));
    else
        P10_OMR.U = (1 << P1_IDX);
}