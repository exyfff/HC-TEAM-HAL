/**
 * @file    hc_hal_dwt.h
 * @brief   HC 团队 HAL DWT 对外接口
 *
 * 提供硬件级精确微秒延迟能力。
 *
 * - Cortex-M3/M4/M7：基于 DWT CYCCNT 硬件周期计数器。
 * - Cortex-M0/M0+：基于 SysTick->VAL 24-bit 递减计数器（需先初始化 SysTick）。
 */

#ifndef HC_HAL_DWT_H
#define HC_HAL_DWT_H

#include "hc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *                        公开 API
 *===========================================================================*/

/**
 * @brief DWT 模块初始化
 *
 * 使能 CoreDebug 调试监视器和 DWT CYCCNT 计数器。
 * 初始化成功后 CYCCNT 将从 0 开始自由递增，每个 CPU 时钟周期加 1。
 *
 * @return HC_S32 成功返回 HC_HAL_OK；
 *                重复初始化返回 HC_HAL_ERR_ALREADY_INIT；
 *                底层硬件未适配返回 HC_ERR_NOT_READY。
 */
HC_S32 HC_HAL_DWT_Init(HC_VOID);

/**
 * @brief 获取当前 DWT CYCCNT 周期计数值
 *
 * - Cortex-M3/M4/M7：返回自 DWT 初始化以来的累计 CPU 周期数
 *   （32-bit 自由计数器，约在 48MHz 下每 ~89 秒溢出一次）。
 * - Cortex-M0/M0+：返回当前 SysTick 重装周期内的已消耗周期数
 *   （24-bit，最大约 16M 周期）。该值在 SysTick 重装时归零，
 *   不是跨重装的累计值，仅适合短区间性能测量。
 *
 * @param p_cycles 输出指针，成功时写入当前周期计数值。
 * @return HC_S32 成功返回 HC_HAL_OK；参数为空返回 HC_HAL_ERR_NULL_PTR；
 *                未初始化返回 HC_HAL_ERR_NOT_INIT。
 */
HC_S32 HC_HAL_DWT_GetCycleCount(HC_U32 *p_cycles);

/**
 * @brief 基于 DWT CYCCNT 的精确微秒阻塞延迟
 *
 * 利用硬件周期计数器实现高精度忙等延迟，不受编译器优化等级影响。
 * 仅适用于裸机场景；若 RTOS 接管任务调度，请改用 OS 提供的延时接口。
 *
 * @param delay_us 延时时长，单位 µs。传入 0 立即返回。
 * @return HC_S32 成功返回 HC_HAL_OK；未初始化返回 HC_HAL_ERR_NOT_INIT。
 */
HC_S32 HC_HAL_DWT_DelayUs(HC_U32 delay_us);

#ifdef __cplusplus
}
#endif

#endif /* HC_HAL_DWT_H */
