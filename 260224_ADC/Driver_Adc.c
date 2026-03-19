/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/

#include "Driver_Adc.h"


/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

App_VadcAutoScan g_VadcAutoScan;
uint32 adcDataResult[8] = {0u,};

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

IfxVadc_Adc_ChannelConfig adcChannelConfig[8];
IfxVadc_Adc_Channel   adcChannel[8];

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

static void Driver_Adc0_Init(void);

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/

static void Driver_Adc0_Init(void)
{
    IfxVadc_Adc_Config adcConfig;
    IfxVadc_Adc_initModuleConfig(&adcConfig, &MODULE_VADC);
    IfxVadc_Adc_initModule(&g_VadcAutoScan.vadc, &adcConfig);

    IfxVadc_Adc_GroupConfig adcGroupConfig;
    IfxVadc_Adc_initGroupConfig(&adcGroupConfig, &g_VadcAutoScan.vadc);

    adcGroupConfig.groupId = IfxVadc_GroupId_4;
    adcGroupConfig.master  = adcGroupConfig.groupId;
    adcGroupConfig.arbiter.requestSlotScanEnabled = TRUE;
    adcGroupConfig.scanRequest.autoscanEnabled = TRUE;
    adcGroupConfig.scanRequest.triggerConfig.gatingMode = IfxVadc_GatingMode_always;

    IfxVadc_Adc_initGroup(&g_VadcAutoScan.adcGroup, &adcGroupConfig);

    /* ---------- 채널 6, 7 둘 다 설정 ---------- */
    for(int ch = 6; ch <= 7; ch++)
    {
        IfxVadc_Adc_initChannelConfig(&adcChannelConfig[ch], &g_VadcAutoScan.adcGroup);

        adcChannelConfig[ch].channelId      = (IfxVadc_ChannelId)ch;
        adcChannelConfig[ch].resultRegister = (IfxVadc_ChannelResult)ch;

        IfxVadc_Adc_initChannel(&adcChannel[ch], &adcChannelConfig[ch]);
    }

    /* ---------- scan에 6번, 7번 둘 다 추가 ---------- */
    unsigned channels = (1 << 6) | (1 << 7);
    unsigned mask     = channels;

    IfxVadc_Adc_setScan(&g_VadcAutoScan.adcGroup, channels, mask);
}


void Driver_Adc_Init(void)
{
    /*ADC0 Converter Init*/
    Driver_Adc0_Init();

    /*ADC0 Converter Start*/
    Driver_Adc0_ConvStart();
}

void Driver_Adc0_ConvStart(void)
{
    /* start autoscan */
    IfxVadc_Adc_startScan(&g_VadcAutoScan.adcGroup);
}

AdcResult Driver_Adc0_DataObtain(void)
{
    AdcResult adcResult;
    for(int ch = 6; ch <= 7; ch++)
    {
        Ifx_VADC_RES result;

        do
        {
            result = IfxVadc_Adc_getResult(&adcChannel[ch]);
        } while (!result.B.VF);

        adcDataResult[ch] = result.B.RESULT;
    }
    adcResult.pot = adcDataResult[7];
    adcResult.light = adcDataResult[6];

    return adcResult;
}
