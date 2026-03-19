#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxPort.h"
#include "IfxPort_PinMap.h"

#include "Driver_Stm.h"

#define SCLK     IfxPort_P00_0
#define RCLK     IfxPort_P00_1
#define DIO      IfxPort_P00_2

#define RED_LED     IfxPort_P10_1
#define BLUE_LED    IfxPort_P10_2

IfxCpu_syncEvent cpuSyncEvent = 0;

/* 7-Segment (Common Anode 기준) */
uint8 _LED_0F[10] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90};

/* 시간 자리 */
volatile uint8 digit_10ms  = 0;   // 맨 아래
volatile uint8 digit_100ms = 0;
volatile uint8 digit_1s    = 0;
volatile uint8 digit_10s   = 0;

/* FND 스캔용 */
volatile uint8 currentDigit = 0;

/* ================= GPIO ================= */

void initGPIO(void)
{
    IfxPort_setPinModeOutput(RED_LED.port, RED_LED.pinIndex,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

    IfxPort_setPinModeOutput(BLUE_LED.port, BLUE_LED.pinIndex,
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

    IfxPort_setPinLow(SCLK.port, SCLK.pinIndex);
    IfxPort_setPinLow(RCLK.port, RCLK.pinIndex);
    IfxPort_setPinLow(DIO.port, DIO.pinIndex);
}

/* ================= SPI Bit Bang ================= */

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

/* ================= FND (1ms에 한 자리만 출력) ================= */

void FndDisplay(void)
{
    uint8 seg;

    switch(currentDigit)
    {
        case 0:
            seg = _LED_0F[digit_10ms];
            send_port(seg, 0x1);
            break;

        case 1:
            seg = _LED_0F[digit_100ms];
            send_port(seg, 0x2);
            break;

        case 2:
            seg = _LED_0F[digit_1s];
            send_port(seg & 0x7F, 0x4);
            break;

        case 3:
            seg = _LED_0F[digit_10s];
            send_port(seg, 0x8);
            break;
    }

    currentDigit++;
    if(currentDigit >= 4)
        currentDigit = 0;
}

/* ================= Tasks ================= */

void AppTask1ms(void)
{
    FndDisplay();
}

void AppTask10ms(void)
{
    digit_10ms++;

    if(digit_10ms >= 10)
    {
        digit_10ms = 0;
        digit_100ms++;

        if(digit_100ms >= 10)
        {
            digit_100ms = 0;
            digit_1s++;

            if(digit_1s >= 10)
            {
                digit_1s = 0;
                digit_10s++;

                if(digit_10s >= 10)
                {
                    digit_10s = 0;
                }
            }
        }
    }
}

void AppTask100ms(void)
{
    static uint8 flag = 0;

    IfxPort_setPinState(RED_LED.port,
                        RED_LED.pinIndex,
                        flag ? IfxPort_State_high : IfxPort_State_low);

    flag ^= 1;
}

void AppTask1000ms(void)
{
    static uint8 flag = 0;

    IfxPort_setPinState(BLUE_LED.port,
                        BLUE_LED.pinIndex,
                        flag ? IfxPort_State_high : IfxPort_State_low);

    flag ^= 1;
}

/* ================= Scheduler ================= */

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
            AppTask100ms();
        }

        if(stSchedulingInfo.u8nuScheduling1000msFlag)
        {
            stSchedulingInfo.u8nuScheduling1000msFlag = 0;
            AppTask1000ms();
        }
    }
}

/* ================= Main ================= */

void core0_main(void)
{
    IfxCpu_enableInterrupts();

    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    initGPIO();
    Driver_Stm_Init();

    while(1)
    {
        AppScheduling();
    }
}
