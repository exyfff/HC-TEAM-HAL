/**
 * @file    hc_hal_dwt.c
 * @brief   HC 团队 HAL DWT 模板实现
 */

/*============================================================================
 *                        包含头文件
 *===========================================================================*/
/* 本文件是主 HAL 模板层，不直接依赖任何厂商 SDK。 */
/* 移植到具体平台时，请在底层硬件接入点替换为目标平台 API。 */
#include "hc_hal_board_cfg.h"
#include "hc_hal_dwt.h"

/*============================================================================
 *                        私有全局变量
 *===========================================================================*/

/** @brief 模块初始化状态标记 */
static volatile HC_Bool_e s_dwt_is_init = HC_FALSE;

/*============================================================================
 *                        私有辅助函数声明
 *===========================================================================*/
static HC_S32 dwt_hw_init(HC_VOID);
static HC_S32 dwt_hw_get_cycle_count(HC_U32 *p_cycles);

/*============================================================================
 *                        私有辅助函数实现
 *===========================================================================*/

static HC_S32 dwt_hw_init(HC_VOID)
{
    /*
     * 待适配: 在此处接入目标芯片的 DWT 初始化。
     * 建议替换流程 (Cortex-M3/M4):
     * 1. 使能 CoreDebug->DEMCR 的 TRCENA 位。
     * 2. 将 DWT->CYCCNT 清零。
     * 3. 使能 DWT->CTRL 的 CYCCNTENA 位。
     *
     * 注意: Cortex-M0/M0+ 不支持 DWT CYCCNT，本模块不可用于这些内核。
     */
    return HC_ERR_NOT_READY;
}

static HC_S32 dwt_hw_get_cycle_count(HC_U32 *p_cycles)
{
    /*
     * 待适配: 读取硬件周期计数器。
     * 典型写法:
     * *p_cycles = DWT->CYCCNT;
     * return HC_HAL_OK;
     */
    HC_UNUSED(p_cycles);
    return HC_ERR_NOT_READY;
}

/*============================================================================
 *                        公开 API 函数实现
 *===========================================================================*/

HC_S32 HC_HAL_DWT_Init(HC_VOID)
{
    HC_S32 ret;

    if (s_dwt_is_init == HC_TRUE) {
        return HC_HAL_ERR_ALREADY_INIT;
    }

    if (HC_HAL_SYSTICK_CPU_HZ == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    s_dwt_is_init = HC_FALSE;

    ret = dwt_hw_init();
    if (ret != HC_HAL_OK) {
        return ret;
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

    return dwt_hw_get_cycle_count(p_cycles);
}

HC_S32 HC_HAL_DWT_DelayUs(HC_U32 delay_us)
{
    HC_U32 start_cycles;
    HC_U32 now_cycles;
    HC_U32 target_cycles;
    HC_S32 ret;

    if (delay_us == 0u) {
        return HC_HAL_OK;
    }

    if (s_dwt_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    target_cycles = (HC_HAL_SYSTICK_CPU_HZ / 1000000u) * delay_us;

    ret = dwt_hw_get_cycle_count(&start_cycles);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    /*
     * 当前是裸机忙等模板。
     * 利用 32-bit 无符号减法自然处理 CYCCNT 溢出回绕。
     * 若迁移到 RTOS，请改为 OS 提供的精确延时接口。
     */
    do {
        ret = dwt_hw_get_cycle_count(&now_cycles);
        if (ret != HC_HAL_OK) {
            return ret;
        }
    } while ((now_cycles - start_cycles) < target_cycles);

    return HC_HAL_OK;
}
