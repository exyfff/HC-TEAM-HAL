/**
 * @file    hc_hal_example_mspm0_systick.c
 * @brief   MSPM0 平台的 HC HAL SysTick 参考实现。
 *
 * 本文件提供 1ms 系统节拍接口，并映射到 MSPM0 的 SysTick 驱动。
 */

#include "ti_dl_lib.h"
#include "hc_hal_example_mspm0_board_cfg.h"
#include "hc_hal_systick.h"

static volatile HC_Bool_e s_systick_is_init = HC_FALSE;
static volatile HC_U32 s_systick_tick_ms = 0u;

static HC_S32 systick_hw_init(HC_VOID)
{
    HC_U32 reload_val;

    if (HC_HAL_SYSTICK_TICK_HZ == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    reload_val = HC_HAL_SYSTICK_CPU_HZ / HC_HAL_SYSTICK_TICK_HZ;
    if (reload_val == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    /*
     * 使用 DriverLib 封装的 SysTick 初始化流程：
     * 1. 配置重装值
     * 2. 打开 SysTick 中断
     * 3. 启动计数器
     */
    if (reload_val > (SysTick_LOAD_RELOAD_Msk + 1u)) {
        return HC_HAL_ERR_INVALID;
    }

    DL_SYSTICK_init(reload_val);
    DL_SYSTICK_enableInterrupt();
    DL_SYSTICK_enable();
    return HC_HAL_OK;
}

HC_S32 HC_HAL_SYSTICK_Init(HC_VOID)
{
    HC_S32 ret;

    if (s_systick_is_init == HC_TRUE) {
        return HC_HAL_ERR_ALREADY_INIT;
    }

    s_systick_is_init = HC_FALSE;
    s_systick_tick_ms = 0u;

    ret = systick_hw_init();
    if (ret != HC_HAL_OK) {
        return ret;
    }

    s_systick_is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_SYSTICK_GetTickMs(HC_U32 *p_tick_ms)
{
    if (p_tick_ms == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (s_systick_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    *p_tick_ms = s_systick_tick_ms;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_SYSTICK_DelayMs(HC_U32 delay_ms)
{
    HC_U32 start_ms;
    HC_U32 now_ms;
    HC_S32 ret;

    if (delay_ms == 0u) {
        return HC_HAL_OK;
    }

    ret = HC_HAL_SYSTICK_GetTickMs(&start_ms);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    do {
        ret = HC_HAL_SYSTICK_GetTickMs(&now_ms);
        if (ret != HC_HAL_OK) {
            return ret;
        }
    } while ((now_ms - start_ms) < delay_ms);

    return HC_HAL_OK;
}

HC_S32 HC_HAL_SYSTICK_DelayUs(HC_U32 delay_us)
{
    HC_U32 loop_per_us;

    if (delay_us == 0u) {
        return HC_HAL_OK;
    }

    if (s_systick_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    loop_per_us = HC_HAL_SYSTICK_CPU_HZ / (HC_HAL_SYSTICK_DELAY_US_CYCLES * 1000000u);
    if (loop_per_us == 0u) {
        loop_per_us = 1u;
    }

    while (delay_us > 0u) {
        volatile HC_U32 loop_cnt = loop_per_us;

        while (loop_cnt > 0u) {
            loop_cnt--;
        }

        delay_us--;
    }

    return HC_HAL_OK;
}

HC_VOID HC_HAL_SYSTICK_IRQHandler(HC_VOID)
{
    if (s_systick_is_init == HC_FALSE) {
        return;
    }

    s_systick_tick_ms++;
}
