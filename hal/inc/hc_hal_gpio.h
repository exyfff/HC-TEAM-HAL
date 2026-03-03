/**
 * @file    hc_hal_gpio.h
 * @brief   HC 团队 HAL GPIO 对外接口
 */

#ifndef HC_HAL_GPIO_H
#define HC_HAL_GPIO_H

#include "hc_common.h"
#include "hc_hal_board_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *                        宏定义与枚举
 *===========================================================================*/

/**
 * @brief 虚拟引脚号类型
 *
 * 具体枚举值必须在板级配置 (board_cfg.h) 中定义 HC_HAL_GPIO_VPin_e 枚举，
 * 并以 VPIN_MAX 标记最大数量。公共头仅做存在性检查。
 *
 * 示例 (board_cfg.h):
 * @code
 * typedef enum {
 *     VPIN_LED_1 = 0,
 *     VPIN_KEY_1 = 1,
 *     VPIN_MAX
 * } HC_HAL_GPIO_VPin_e;
 * @endcode
 */
#ifndef VPIN_MAX
#error "HC_HAL_GPIO_VPin_e enum with VPIN_MAX must be defined in board_cfg.h"
#endif

/**
 * @brief 引脚电平状态定义
 */
typedef enum {
    HC_PIN_RESET = 0, /* 低电平 (0) */
    HC_PIN_SET = 1    /* 高电平 (1) */
} HC_HAL_GPIO_PinState_e;

/**
 * @brief 中断向量名重定向映射
 *
 * 以下宏必须在板级配置 (board_cfg.h) 中根据目标平台定义，例如：
 * - MSPM0: #define HC_HAL_GPIOA_IRQHandler GPIOA_IRQHandler
 * - STM32: #define HC_HAL_GPIOA_IRQHandler EXTI15_10_IRQHandler
 */
#ifndef HC_HAL_GPIOA_IRQHandler
#error "HC_HAL_GPIOA_IRQHandler must be defined in board_cfg.h"
#endif

#ifndef HC_HAL_GPIOB_IRQHandler
#error "HC_HAL_GPIOB_IRQHandler must be defined in board_cfg.h"
#endif

/*============================================================================
 *                        公开 API
 *===========================================================================*/

/**
 * @brief GPIO 模块初始化
 * 
 * 该函数负责遍历内部映射表，一次性完成所有虚拟引脚的方向、上下拉、初始电平及中断使能配置。
 * 
 * @return HC_S32 初始化状态，成功返回 HC_HAL_OK，失败返回负值错误码。
 */
HC_S32 HC_HAL_GPIO_Init(HC_VOID);

/**
 * @brief 设置引脚为高电平
 * 
 * @param vpin 虚拟引脚号，参考 @ref HC_HAL_GPIO_VPin_e
 * @return HC_S32 成功返回 HC_HAL_OK，参数错误或模块未初始化返回错误码。
 */
HC_S32 HC_HAL_GPIO_SetPin(HC_HAL_GPIO_VPin_e vpin);

/**
 * @brief 设置引脚为低电平
 * 
 * @param vpin 虚拟引脚号，参考 @ref HC_HAL_GPIO_VPin_e
 * @return HC_S32 成功返回 HC_HAL_OK，错误返回负值。
 */
HC_S32 HC_HAL_GPIO_ResetPin(HC_HAL_GPIO_VPin_e vpin);

/**
 * @brief 翻转引脚电平
 * 
 * @param vpin 虚拟引脚号，参考 @ref HC_HAL_GPIO_VPin_e
 * @return HC_S32 成功返回 HC_HAL_OK，错误返回负值。
 */
HC_S32 HC_HAL_GPIO_TogglePin(HC_HAL_GPIO_VPin_e vpin);

/**
 * @brief 读取引脚当前物理电平状态
 * 
 * @param vpin 虚拟引脚号，参考 @ref HC_HAL_GPIO_VPin_e
 * @param p_state 输出指针，成功时写入 HC_PIN_SET 或 HC_PIN_RESET。
 * @return HC_S32 成功返回 HC_HAL_OK，失败返回负值 HC_ERR_xxx。
 */
HC_S32 HC_HAL_GPIO_ReadPin(HC_HAL_GPIO_VPin_e vpin, HC_HAL_GPIO_PinState_e *p_state);

/*============================================================================
 *                        中断处理与回调
 *===========================================================================*/

/**
 * @brief PORT A 中断处理入口
 */
HC_VOID HC_HAL_GPIOA_IRQHandler(HC_VOID);

/**
 * @brief PORT B 中断处理入口
 */
HC_VOID HC_HAL_GPIOB_IRQHandler(HC_VOID);

/**
 * @brief GPIO 中断用户回调函数
 * 
 * 该函数在中断处理程序中被调用。这是一个弱符号 (weak) 定义，用户应在业务层自行重写该函数。
 * 当前模板仅供裸机参考；若迁移到 RTOS，建议只在此处投递事件，不要直接执行耗时业务。
 * 
 * @param vpin 触发中断的虚拟引脚号。
 */
HC_VOID HC_HAL_GPIO_Callback(HC_HAL_GPIO_VPin_e vpin);

#ifdef __cplusplus
}
#endif

#endif /* HC_HAL_GPIO_H */
