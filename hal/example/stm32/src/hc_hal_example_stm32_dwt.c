/**
 * @file    hc_hal_example_stm32_dwt.c
 * @brief   HC 团队 HAL DWT 的 STM32 参考实现
 *
 * 本文件使用 ARM Cortex-M3/M4 标准 CMSIS 寄存器 (CoreDebug / DWT)
 * 实现基于 CYCCNT 的硬件精确微秒延迟。
 *
 * 适用范围：STM32F1/F2/F3/F4/F7/H7/G4/L4 等 Cortex-M3/M4/M7 系列。
 * 不适用于 STM32G0/L0/C0 等 Cortex-M0/M0+ 系列（无 DWT CYCCNT）。
 */

#include "stm32xxxx_hal.h"
#include "hc_hal_example_stm32_board_cfg.h"
#include "hc_hal_dwt.h"

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

    if (SystemCoreClock == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    /* 使能 CoreDebug 跟踪，是访问 DWT 寄存器的前提。 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* 清零并启动周期计数器。 */
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /*
     * 部分 Cortex-M 实现中 DWT CYCCNT 可能不存在（如精简版 M3）。
     * 写入后回读验证计数器是否正常递增。
     */
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0u) {
        return HC_ERR_NOT_READY;
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

    *p_cycles = DWT->CYCCNT;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_DWT_DelayUs(HC_U32 delay_us)
{
    HC_U32 start_cycles;
    HC_U32 target_cycles;

    if (delay_us == 0u) {
        return HC_HAL_OK;
    }

    if (s_dwt_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    /*
     * SystemCoreClock 是 STM32 HAL 提供的全局变量，
     * 在 SystemInit() / HAL_RCC_ClockConfig() 后自动更新。
     */
    target_cycles = (SystemCoreClock / 1000000u) * delay_us;
    start_cycles = DWT->CYCCNT;

    /*
     * 利用 32-bit 无符号减法自然处理 CYCCNT 溢出回绕：
     * 当 CYCCNT 从 0xFFFFFFFF 翻转到 0 时，
     * (now - start) 的差值仍然正确（无符号算术溢出语义）。
     */
    while ((DWT->CYCCNT - start_cycles) < target_cycles) {
        /* 忙等 */
    }

    return HC_HAL_OK;
}
