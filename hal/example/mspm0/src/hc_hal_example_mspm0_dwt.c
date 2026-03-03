/**
 * @file    hc_hal_example_mspm0_dwt.c
 * @brief   HC 团队 HAL DWT 的 MSPM0 参考实现
 *
 * Cortex-M0+ 不支持 DWT CYCCNT 硬件计数器。
 * 本文件使用 SysTick->VAL（24-bit 递减计数器）作为替代方案，
 * 在 SysTick 已初始化的前提下提供硬件级精确微秒延迟。
 *
 * 前置依赖：HC_HAL_SYSTICK_Init() 必须在 HC_HAL_DWT_Init() 之前调用。
 */

#include "ti_dl_lib.h"
#include "hc_hal_example_mspm0_board_cfg.h"
#include "hc_hal_dwt.h"
#include "hc_hal_systick.h"

/*============================================================================
 *                        私有全局变量
 *===========================================================================*/

static volatile HC_Bool_e s_dwt_is_init = HC_FALSE;

/*============================================================================
 *                        公开 API 函数实现
 *===========================================================================*/

HC_S32 HC_HAL_DWT_Init(HC_VOID)
{
    if (s_dwt_is_init == HC_TRUE) {
        return HC_HAL_ERR_ALREADY_INIT;
    }

    /*
     * M0+ 没有 DWT CYCCNT，本实现依赖 SysTick 的 24-bit 递减计数器。
     * SysTick 必须已经初始化并正在运行。
     */
    if ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) == 0u) {
        return HC_HAL_ERR_NOT_INIT;
    }

    s_dwt_is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_DWT_GetCycleCount(HC_U32 *p_cycles)
{
    if (p_cycles == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (s_dwt_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    /*
     * SysTick->VAL 是递减计数器 (LOAD → 0)，此处反转为递增语义：
     * cycle_count = LOAD - VAL，使返回值的含义与 DWT->CYCCNT 一致。
     *
     * 注意：该值仅在单次 SysTick 重装周期内有效（24-bit，最大约 16M 周期），
     * 跨重装的精确度由 DelayUs 内部的多重装补偿逻辑保证。
     */
    *p_cycles = SysTick->LOAD - SysTick->VAL;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_DWT_DelayUs(HC_U32 delay_us)
{
    HC_U32 ticks_needed;
    HC_U32 reload;
    HC_U32 elapsed;
    HC_U32 old_val;
    HC_U32 new_val;

    if (delay_us == 0u) {
        return HC_HAL_OK;
    }

    if (s_dwt_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    /*
     * 使用 SysTick->VAL 的递减特性实现精确 µs 延迟。
     *
     * SysTick 是 24-bit 递减计数器：LOAD → 0 → LOAD → ...
     * 每次从 0 回到 LOAD 时会触发溢出（COUNTFLAG 置位 / 中断触发）。
     *
     * 算法：
     * 1. 记录起始 VAL 值。
     * 2. 每次读取新 VAL 值，如果 new_val > old_val 说明发生了回绕。
     * 3. 累加已消耗的周期数，直到达到目标值。
     */
    reload = SysTick->LOAD;
    ticks_needed = (HC_HAL_SYSTICK_CPU_HZ / 1000000u) * delay_us;
    elapsed = 0u;
    old_val = SysTick->VAL;

    while (elapsed < ticks_needed) {
        new_val = SysTick->VAL;

        if (new_val <= old_val) {
            /* 正常递减，未回绕 */
            elapsed += (old_val - new_val);
        } else {
            /* 回绕：old_val → 0 → LOAD → new_val */
            elapsed += (old_val + (reload - new_val));
        }

        old_val = new_val;
    }

    return HC_HAL_OK;
}
