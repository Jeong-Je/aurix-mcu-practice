#include "Driver_Stm.h"

#include "IfxStm.h"
#include "IfxSrc.h"
#include "IfxCpu.h"

typedef struct
{
        Ifx_STM             *stmSfr;
        IfxStm_CompareConfig stmConfig;
        volatile uint32      counter;
} App_Stm;


App_Stm g_Stm;
uint32 u32nuCounter1ms = 0u;
SchedulingFlag stSchedulingInfo;

IFX_INTERRUPT(STM_Int0Handler, 0, 100);

void STM_Int0Handler(void) {
    IfxCpu_enableInterrupts();

    IfxStm_clearCompareFlag(g_Stm.stmSfr, g_Stm.stmConfig.comparator);
    IfxStm_increaseCompare(g_Stm.stmSfr,
                           g_Stm.stmConfig.comparator,
                           100000u);

    u32nuCounter1ms++;

    if((u32nuCounter1ms % 1) == 0u)
        stSchedulingInfo.u8nuScheduling1msFlag = 1u;

    if ((u32nuCounter1ms % 10u) == 0u)
        stSchedulingInfo.u8nuScheduling10msFlag = 1u;

    if ((u32nuCounter1ms % 100u) == 0u)
        stSchedulingInfo.u8nuScheduling100msFlag = 1u;

    if ((u32nuCounter1ms % 1000u) == 0u)
        stSchedulingInfo.u8nuScheduling1000msFlag = 1u;
}

void  Driver_Stm_Init(void)
{
    boolean interruptState = IfxCpu_disableInterrupts();

    g_Stm.stmSfr = &MODULE_STM0;

    IfxStm_initCompareConfig(&g_Stm.stmConfig);

    g_Stm.stmConfig.triggerPriority = 100u;
    g_Stm.stmConfig.typeOfService   = IfxSrc_Tos_cpu0;
    g_Stm.stmConfig.ticks           = 100000u;

    IfxStm_initCompare(g_Stm.stmSfr, &g_Stm.stmConfig);

    IfxCpu_restoreInterrupts(interruptState);
}


