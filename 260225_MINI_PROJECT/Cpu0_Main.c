#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxPort.h"
#include "IfxPort_PinMap.h"
#include "IfxScuEru.h"

#include "Driver_Stm.h"
#include "Driver_Adc.h"

#define SCLK     IfxPort_P00_0 // 25
#define RCLK     IfxPort_P00_1 // 27
#define DIO      IfxPort_P00_2 // 29

#define RED_LED  IfxPort_P10_1
#define BLUE_LED IfxPort_P10_2

#define RED_RGB     IfxPort_P02_7
#define GREEN_RGB   IfxPort_P10_5
#define BLUE_RGB    IfxPort_P10_3

uint8 _LED_0F[10] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90};

IfxCpu_syncEvent cpuSyncEvent = 0;

typedef enum
{
    STATE_NORMAL = 0,
    STATE_CRUISE,
    STATE_EMERGENCY
} VehicleState;

volatile VehicleState g_state = STATE_NORMAL;
volatile uint8 g_speed = 0;
AdcResult adcResult;

IFX_INTERRUPT(SW1_ISR, 0, 0x10u);
IFX_INTERRUPT(SW2_ISR, 0, 0x20u);

// 일반 모드 <-> 크루즈 모드 전환하는 ISR
void SW1_ISR(void)
{
    IfxScuEru_clearEventFlag(IfxScuEru_InputChannel_3);
    if(g_state == STATE_NORMAL) {
        g_state = STATE_CRUISE;
        g_speed = 70;
    } else if (g_state == STATE_CRUISE) {
        g_state = STATE_NORMAL;
    }
}

// 일반 모드 <-> 비상 모드 전환하는 ISR
void SW2_ISR(void)
{
    IfxScuEru_clearEventFlag(IfxScuEru_InputChannel_2);
    if(g_state == STATE_NORMAL) {
        g_state = STATE_EMERGENCY;
        g_speed = 0;
    } else if (g_state == STATE_EMERGENCY) {
        g_state = STATE_NORMAL;
    }
}


void initGPIO(){
    IfxPort_setPinModeOutput(RED_LED.port, RED_LED.pinIndex,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

   IfxPort_setPinModeOutput(BLUE_LED.port, BLUE_LED.pinIndex,
                            IfxPort_OutputMode_pushPull,
                            IfxPort_OutputIdx_general);

   IfxPort_setPinModeOutput(RED_RGB.port, RED_RGB.pinIndex,
                            IfxPort_OutputMode_pushPull,
                            IfxPort_OutputIdx_general);

   IfxPort_setPinModeOutput(GREEN_RGB.port, GREEN_RGB.pinIndex,
                            IfxPort_OutputMode_pushPull,
                            IfxPort_OutputIdx_general);

   IfxPort_setPinModeOutput(BLUE_RGB.port, BLUE_RGB.pinIndex,
                            IfxPort_OutputMode_pushPull,
                            IfxPort_OutputIdx_general);

    IfxPort_setPinModeOutput(SCLK.port, SCLK.pinIndex,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

   IfxPort_setPinModeOutput(RCLK.port, RCLK.pinIndex,
                            IfxPort_OutputMode_pushPull,
                            IfxPort_OutputIdx_general);

   IfxPort_setPinModeOutput(DIO.port, DIO.pinIndex,
                            IfxPort_OutputMode_pushPull,
                            IfxPort_OutputIdx_general);

   IfxPort_setPinLow(RED_LED.port, RED_LED.pinIndex);
   IfxPort_setPinLow(BLUE_LED.port, BLUE_LED.pinIndex);

   IfxPort_setPinLow(RED_RGB.port, RED_RGB.pinIndex);
   IfxPort_setPinLow(GREEN_RGB.port, GREEN_RGB.pinIndex);
   IfxPort_setPinLow(BLUE_RGB.port, BLUE_RGB.pinIndex);

   IfxPort_setPinLow(SCLK.port, SCLK.pinIndex);
   IfxPort_setPinLow(RCLK.port, RCLK.pinIndex);
   IfxPort_setPinLow(DIO.port, DIO.pinIndex);
}

void send(uint8 data)
{
    for(int i=0;i<8;i++)
    {
        if(data & 0x80)
            IfxPort_setPinHigh(DIO.port, DIO.pinIndex);
        else
            IfxPort_setPinLow(DIO.port, DIO.pinIndex);

        data <<= 1;

        IfxPort_setPinLow(SCLK.port, SCLK.pinIndex);
        IfxPort_setPinHigh(SCLK.port, SCLK.pinIndex);
    }
}

void send_port(uint8 seg, uint8 digit)
{
    send(seg);
    send(digit);

    IfxPort_setPinLow(RCLK.port, RCLK.pinIndex);
    IfxPort_setPinHigh(RCLK.port, RCLK.pinIndex);
}

sint32 map(sint32 x,
                sint32 in_min,
                sint32 in_max,
                sint32 out_min,
                sint32 out_max)
{
    return ( (x - in_min) * (out_max - out_min)
            / (in_max - in_min) )
            + out_min;
}

void AppTask1ms(void)
{
    static uint8 digit = 0;

    int n1 = g_speed % 10;
    int n2 = (g_speed / 10) % 10;
    int n3 = (g_speed / 100) % 10;
    int n4 = (g_speed / 1000) % 10;

    switch(digit)
    {
        case 0: send_port(_LED_0F[n1], 0x1); break;
        case 1: send_port(_LED_0F[n2], 0x2); break;
        case 2: send_port(_LED_0F[n3], 0x4); break;
        case 3: send_port(_LED_0F[n4], 0x8); break;
    }

    digit++;
    if(digit >= 4) digit = 0;
}

void AppTask10ms(void) {
    adcResult = Driver_Adc0_DataObtain();

    // 조도 센서로 읽어서 주변이 어두우면 블루 LED 점등
    if(adcResult.light < 3000) {
        IfxPort_setPinHigh(BLUE_LED.port, BLUE_LED.pinIndex);
    } else {
        IfxPort_setPinLow(BLUE_LED.port, BLUE_LED.pinIndex);
    }

    if(g_state == STATE_NORMAL) g_speed = map(adcResult.pot, 0, 4095, 0, 200);

    // 차량의 속도가 120 이상일 경우 레드 LED 점등
    if(g_speed >= 120) {
        IfxPort_setPinHigh(RED_LED.port, RED_LED.pinIndex);
        makeSound(5);
    } else {
        IfxPort_setPinLow(RED_LED.port, RED_LED.pinIndex);
        stopSound();
    }

    // 차량의 상태에 따라서 RGB 색 구분해서 점등
    // Normal: Green 점등, Cruise: Blue 점등, Emergency: Red 점등
    if(g_state == STATE_NORMAL) {
        IfxPort_setPinLow(RED_RGB.port, RED_RGB.pinIndex);
        IfxPort_setPinHigh(GREEN_RGB.port, GREEN_RGB.pinIndex);
        IfxPort_setPinLow(BLUE_RGB.port, BLUE_RGB.pinIndex);
    } else if(g_state == STATE_CRUISE) {
        IfxPort_setPinLow(RED_RGB.port, RED_RGB.pinIndex);
        IfxPort_setPinLow(GREEN_RGB.port, GREEN_RGB.pinIndex);
        IfxPort_setPinHigh(BLUE_RGB.port, BLUE_RGB.pinIndex);
    } else if(g_state == STATE_EMERGENCY) {
        IfxPort_setPinHigh(RED_RGB.port, RED_RGB.pinIndex);
        IfxPort_setPinLow(GREEN_RGB.port, GREEN_RGB.pinIndex);
        IfxPort_setPinLow(BLUE_RGB.port, BLUE_RGB.pinIndex);
    }


    Driver_Adc0_ConvStart();
}

void AppScheduling(void)
{
    if(stSchedulingInfo.u8nuScheduling1msFlag)
    {
        stSchedulingInfo.u8nuScheduling1msFlag = 0;
        AppTask1ms();

        if(stSchedulingInfo.u8nuScheduling10msFlag)
        {
            stSchedulingInfo.u8nuScheduling10msFlag = 0;
            AppTask10ms();
        }

        if(stSchedulingInfo.u8nuScheduling100msFlag)
        {
            stSchedulingInfo.u8nuScheduling100msFlag = 0;
            //AppTask100ms();
        }

        if(stSchedulingInfo.u8nuScheduling1000msFlag)
        {
            stSchedulingInfo.u8nuScheduling1000msFlag = 0;
            //AppTask1000ms();
        }
    }
}

void initERU(void)
{
    /* -------------------- SW1 : P02.0 -------------------- */

    IfxPort_setPinModeInput(IfxPort_P02_0.port,
                            IfxPort_P02_0.pinIndex,
                            IfxPort_InputMode_pullUp);

    // Input Channel 3 선택
    IfxScuEru_selectExternalInput(IfxScuEru_InputChannel_3,
                                  IfxScuEru_ExternalInputSelection_2);

    IfxScuEru_enableFallingEdgeDetection(IfxScuEru_InputChannel_3);
    IfxScuEru_disableRisingEdgeDetection(IfxScuEru_InputChannel_3);

    IfxScuEru_enableTriggerPulse(IfxScuEru_InputChannel_3);

    // Output Channel 1에 연결
    IfxScuEru_connectTrigger(IfxScuEru_InputChannel_3,
                             IfxScuEru_InputNodePointer_1);

    IfxScuEru_setInterruptGatingPattern(
        IfxScuEru_OutputChannel_1,
        IfxScuEru_InterruptGatingPattern_alwaysActive);

    IfxScuEru_setFlagPatternDetection(
        IfxScuEru_OutputChannel_1,
        IfxScuEru_InputChannel_3,
        TRUE);

    IfxScuEru_clearEventFlag(IfxScuEru_InputChannel_3);

    // ERU1 → Priority 0x10
    IfxSrc_init(&SRC_SCU_SCU_ERU1, IfxSrc_Tos_cpu0, 0x10u);
    IfxSrc_enable(&SRC_SCU_SCU_ERU1);


    /* -------------------- SW2 : P02.1 -------------------- */

    IfxPort_setPinModeInput(IfxPort_P02_1.port,
                            IfxPort_P02_1.pinIndex,
                            IfxPort_InputMode_pullUp);

    // Input Channel 2 선택
    IfxScuEru_selectExternalInput(IfxScuEru_InputChannel_2,
                                  IfxScuEru_ExternalInputSelection_1);

    IfxScuEru_enableFallingEdgeDetection(IfxScuEru_InputChannel_2);
    IfxScuEru_disableRisingEdgeDetection(IfxScuEru_InputChannel_2);

    IfxScuEru_enableTriggerPulse(IfxScuEru_InputChannel_2);

    // Output Channel 2에 연결
    IfxScuEru_connectTrigger(IfxScuEru_InputChannel_2,
                             IfxScuEru_InputNodePointer_2);

    IfxScuEru_setInterruptGatingPattern(
        IfxScuEru_OutputChannel_2,
        IfxScuEru_InterruptGatingPattern_alwaysActive);

    IfxScuEru_setFlagPatternDetection(
        IfxScuEru_OutputChannel_2,
        IfxScuEru_InputChannel_2,
        TRUE);

    IfxScuEru_clearEventFlag(IfxScuEru_InputChannel_2);

    // ERU2 → Priority 0x20
    IfxSrc_init(&SRC_SCU_SCU_ERU2, IfxSrc_Tos_cpu0, 0x20u);
    IfxSrc_enable(&SRC_SCU_SCU_ERU2);
}




void core0_main(void)
{
    IfxCpu_enableInterrupts();
    
    /* !!WATCHDOG0 AND SAFETY WATCHDOG ARE DISABLED HERE!!
     * Enable the watchdogs and service them periodically if it is required
     */
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());
    
    /* Wait for CPU sync event */
    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    Driver_Adc_Init();
    Driver_Stm_Init();
    initGPIO();
    initERU();
    initGtmTomPwm();
        
    while(1)
    {
        AppScheduling();
    }
}
