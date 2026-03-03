/**
 * @file    hc_hal_example_stm32_systick.c
 * @brief   HC 团队 HAL SYSTICK 的 STM32 参考实现
 *
 * 本文件使用标准 CMSIS `SysTick_Config()` 接入 1ms 系统节拍。
 */

#include "stm32xxxx_hal.h"
#include "hc_hal_example_stm32_board_cfg.h"
#include "hc_hal_systick.h"

static volatile HC_Bool_e s_systick_is_init = HC_FALSE;
static volatile HC_U32 s_systick_tick_ms = 0u;

HC_S32 HC_HAL_SYSTICK_Init(HC_VOID)
{
    HC_U32 reload_val;

    if (s_systick_is_init == HC_TRUE) {
        return HC_HAL_ERR_ALREADY_INIT;
    }

    if (HC_HAL_EXAMPLE_STM32_SYSTICK_TICK_HZ == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    reload_val = SystemCoreClock / HC_HAL_EXAMPLE_STM32_SYSTICK_TICK_HZ;
    if ((reload_val == 0u) || (SysTick_Config(reload_val) != 0u)) {
        return HC_ERR_NOT_READY;
    }

    s_systick_tick_ms = 0u;
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

    loop_per_us = SystemCoreClock / (3u * 1000000u);
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
    /*
     * STM32 HAL 库内部使用独立的 uwTick 计数器（由 HAL_IncTick 驱动），
     * 它服务于 HAL_Delay / HAL_GetTick 等官方库超时接口。
     * HC HAL 的 s_systick_tick_ms 是我们自己的独立计数器，
     * 服务于 HC_HAL_SYSTICK_GetTickMs / HC_HAL_SYSTICK_DelayMs 等自有接口。
     * 两套计数器共享同一个 SysTick 中断源，互不干扰。
     */
    HAL_IncTick();

    if (s_systick_is_init == HC_TRUE) {
        s_systick_tick_ms++;
    }

    HAL_SYSTICK_IRQHandler();
}
