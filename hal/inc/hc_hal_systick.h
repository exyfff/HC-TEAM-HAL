/**
 * @file    hc_hal_systick.h
 * @brief   HC 团队 HAL SYSTICK 对外接口
 */

#ifndef HC_HAL_SYSTICK_H
#define HC_HAL_SYSTICK_H

#include "hc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *                        宏定义
 *===========================================================================*/

/* 启动文件中的系统节拍中断入口通过宏映射到统一的 HAL 接口。 */
#define HC_HAL_SYSTICK_IRQHandler SysTick_Handler

/*============================================================================
 *                        公开 API
 *===========================================================================*/

/**
 * @brief 初始化系统节拍模块
 *
 * 当前模板固定使用 1ms 作为系统时间基准。
 * 注意：该实现仅供裸机参考；若 RTOS 已接管 SysTick，则不能直接复用本实现。
 *
 * @return HC_S32 成功返回 HC_HAL_OK；若底层节拍硬件尚未适配，返回 HC_ERR_NOT_READY。
 */
HC_S32 HC_HAL_SYSTICK_Init(HC_VOID);

/**
 * @brief 获取自初始化以来的毫秒计数
 *
 * @param p_tick_ms 输出指针，成功时写入当前毫秒计数。
 * @return HC_S32 成功返回 HC_HAL_OK。
 */
HC_S32 HC_HAL_SYSTICK_GetTickMs(HC_U32 *p_tick_ms);

/**
 * @brief 裸机阻塞式毫秒延时
 *
 * 该接口通过忙等等待系统节拍推进，不适用于 RTOS 任务上下文中的让出式延时需求。
 *
 * @param delay_ms 延时时长，单位 ms。
 * @return HC_S32 成功返回 HC_HAL_OK。
 */
HC_S32 HC_HAL_SYSTICK_DelayMs(HC_U32 delay_ms);

/**
 * @brief 裸机阻塞式微秒延时
 *
 * 该接口仅提供近似的忙等模板，不保证跨平台的高精度延时。
 * 若后续迁移到 RTOS 或对精度有更高要求，应改用专用定时器。
 *
 * @param delay_us 延时时长，单位 us。
 * @return HC_S32 成功返回 HC_HAL_OK。
 */
HC_S32 HC_HAL_SYSTICK_DelayUs(HC_U32 delay_us);

/**
 * @brief 系统节拍中断处理入口
 */
HC_VOID HC_HAL_SYSTICK_IRQHandler(HC_VOID);

#ifdef __cplusplus
}
#endif

#endif /* HC_HAL_SYSTICK_H */
