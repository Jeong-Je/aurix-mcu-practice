
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxPort.h"
#include "IfxPort_PinMap.h"

#include "Driver_Adc.h"
#include "Driver_Stm.h"

#define RED_LED     IfxPort_P10_1
#define BLUE_LED     IfxPort_P10_2

#define RED_RGB     IfxPort_P02_7
#define GREEN_RGB   IfxPort_P10_5
#define BLUE_RGB    IfxPort_P10_3

#define SCLK     IfxPort_P00_0
#define RCLK     IfxPort_P00_1
#define DIO      IfxPort_P00_2

uint8 _LED_0F[10] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90};

uint16 potVal;
uint16 arrayPot[100] = {0, };
uint8 potIndex = 0;

IfxCpu_syncEvent cpuSyncEvent = 0;

AdcResult adcResult;

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

void AppTask1ms(void)
{
    static uint8 digit = 0;

    int n1 = potVal % 10;
    int n2 = (potVal / 10) % 10;
    int n3 = (potVal / 100) % 10;
    int n4 = (potVal / 1000) % 10;

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

    if(adcResult.light < 3000) {
        IfxPort_setPinHigh(RED_LED.port, RED_LED.pinIndex);
        IfxPort_setPinLow(BLUE_LED.port, BLUE_LED.pinIndex);
    } else {
        IfxPort_setPinLow(RED_LED.port, RED_LED.pinIndex);
        IfxPort_setPinHigh(BLUE_LED.port, BLUE_LED.pinIndex);
    }

    arrayPot[potIndex++] = adcResult.pot;
    if(potIndex == 100) potIndex = 0;

    int sum = 0;
    for(int i=0;i<100;i++){
        sum += arrayPot[i];
    }

    potVal = sum / 100;

    if(potVal < 1300) {
        IfxPort_setPinHigh(RED_RGB.port, RED_RGB.pinIndex);
        IfxPort_setPinLow(GREEN_RGB.port, GREEN_RGB.pinIndex);
        IfxPort_setPinLow(BLUE_RGB .port, BLUE_RGB .pinIndex);
    } else if(potVal < 2600) {
        IfxPort_setPinLow(RED_RGB.port, RED_RGB.pinIndex);
        IfxPort_setPinHigh(GREEN_RGB.port, GREEN_RGB.pinIndex);
        IfxPort_setPinLow(BLUE_RGB .port, BLUE_RGB .pinIndex);
    } else {
        IfxPort_setPinLow(RED_RGB.port, RED_RGB.pinIndex);
        IfxPort_setPinLow(GREEN_RGB.port, GREEN_RGB.pinIndex);
        IfxPort_setPinHigh(BLUE_RGB .port, BLUE_RGB .pinIndex);
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



void initGPIO(void)
{
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

    IfxPort_setPinModeOutput(RED_LED.port, RED_LED.pinIndex,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

    IfxPort_setPinModeOutput(BLUE_LED.port, BLUE_LED.pinIndex,
                             IfxPort_OutputMode_pushPull,
                             IfxPort_OutputIdx_general);

    IfxPort_setPinLow(RED_LED.port, RED_LED.pinIndex);
    IfxPort_setPinLow(BLUE_LED.port, BLUE_LED.pinIndex);


    IfxPort_setPinLow(SCLK.port, SCLK.pinIndex);
    IfxPort_setPinLow(RCLK.port, RCLK.pinIndex);
    IfxPort_setPinLow(DIO.port, DIO.pinIndex);


    IfxPort_setPinLow(RED_RGB.port, RED_RGB.pinIndex);
    IfxPort_setPinLow(GREEN_RGB.port, GREEN_RGB.pinIndex);
    IfxPort_setPinLow(BLUE_RGB.port, BLUE_RGB.pinIndex);
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
        
    while(1)
    {
        AppScheduling();
    }
}
