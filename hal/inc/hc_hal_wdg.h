/**
 * @file    hc_hal_wdg.h
 * @brief   HC 团队 HAL 看门狗对外接口
 */

#ifndef HC_HAL_WDG_H
#define HC_HAL_WDG_H

#include "hc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *                        公开 API
 *===========================================================================*/

/**
 * @brief 看门狗模块初始化
 * 
 * 完成看门狗超时周期配置并直接激活看门狗引擎。
 * 注意：一旦初始化成功，看门狗将无法通过软件指令强行关闭（安全策略）。
 * 
 * @return HC_S32 成功返回 HC_HAL_OK；若底层硬件未适配，返回 HC_ERR_NOT_READY。
 */
HC_S32 HC_HAL_WDG_Init(HC_VOID);

/**
 * @brief 喂狗操作 (服务看门狗)
 * 
 * 必须在配置的超时周期内定期调用，否则会导致系统复位。
 * 
 * @return HC_S32 成功返回 HC_HAL_OK；若底层硬件未适配，返回 HC_ERR_NOT_READY。
 */
HC_S32 HC_HAL_WDG_Feed(HC_VOID);

/**
 * @brief 查询最近一次系统复位是否由看门狗引起
 * 
 * 返回的是“冷启动后第一次初始化时锁定”的复位来源缓存，
 * 而不是每次调用时都实时读取硬件复位状态寄存器。
 *
 * @param p_is_wdg_reset 输出指针，成功后存储复位状态 (HC_TRUE/HC_FALSE)
 * @return HC_S32 成功返回 HC_HAL_OK。
 */
HC_S32 HC_HAL_WDG_GetResetFlag(HC_Bool_e *p_is_wdg_reset);

#ifdef __cplusplus
}
#endif

#endif /* HC_HAL_WDG_H */
