/**
 * @file    hc_hal_board_cfg.h
 * @brief   HC 团队 HAL 通用模板板级配置
 *
 * 本文件是主 HAL 模板层的占位配置，不绑定任何厂商 SDK。
 * 移植到具体平台时，建议先复制本模板，再按目标芯片补齐实例使能、基址、
 * 引脚资源和系统时基参数。
 *
 * 约束：
 * 1. 公共头文件 (hal/inc/*.h) 不得包含本文件。
 * 2. 本文件可被 HAL 实现层 (.c 文件) 包含。
 * 3. 本模板默认不启用任何 UART/I2C 实例，避免误导性“看起来可用”。
 */

#ifndef HC_HAL_BOARD_CFG_H
#define HC_HAL_BOARD_CFG_H

#include "hc_common.h"

/*============================================================================
 *                        GPIO 模板配置
 *===========================================================================*/

/* GPIO 模板默认使用单端口、每端口 32 位的中断查表维度。 */
#define HC_HAL_GPIO_PORT_NUM 1u
#define HC_HAL_GPIO_PIN_NUM  32u

/* 虚拟引脚号枚举：移植时根据产品实际 GPIO 需求修改。 */
typedef enum {
    VPIN_LED_1 = 0,
    VPIN_KEY_1 = 1,
    VPIN_MAX
} HC_HAL_GPIO_VPin_e;

/*
 * GPIO 中断向量名占位：移植时按目标平台的中断向量表替换。
 * - MSPM0 示例: #define HC_HAL_GPIOA_IRQHandler GPIOA_IRQHandler
 * - STM32 示例: #define HC_HAL_GPIOA_IRQHandler EXTI15_10_IRQHandler
 */
#define HC_HAL_GPIOA_IRQHandler GPIOA_INT_IRQHandler
#define HC_HAL_GPIOB_IRQHandler GPIOB_INT_IRQHandler

/*============================================================================
 *                        UART 模板配置
 *===========================================================================*/

/* 默认不启用实例；移植时按目标平台资源改为 1。 */

/*
 * UART 中断向量名占位：移植时按目标平台替换。
 */
#define HC_HAL_UART0_IRQHandler UART0_INT_IRQHandler
#define HC_HAL_UART1_IRQHandler UART1_INT_IRQHandler

#define HC_HAL_UART0_ENABLE   0u
#define HC_HAL_UART0_BASE     HC_NULL_PTR
#define HC_HAL_UART0_TX_PORT  HC_NULL_PTR
#define HC_HAL_UART0_TX_PIN   0u
#define HC_HAL_UART0_TX_IOMUX 0u
#define HC_HAL_UART0_RX_PORT  HC_NULL_PTR
#define HC_HAL_UART0_RX_PIN   0u
#define HC_HAL_UART0_RX_IOMUX 0u

#define HC_HAL_UART1_ENABLE   0u
#define HC_HAL_UART1_BASE     HC_NULL_PTR
#define HC_HAL_UART1_TX_PORT  HC_NULL_PTR
#define HC_HAL_UART1_TX_PIN   0u
#define HC_HAL_UART1_TX_IOMUX 0u
#define HC_HAL_UART1_RX_PORT  HC_NULL_PTR
#define HC_HAL_UART1_RX_PIN   0u
#define HC_HAL_UART1_RX_IOMUX 0u

/*============================================================================
 *                        I2C 模板配置
 *===========================================================================*/

#define HC_HAL_I2C0_ENABLE    0u
#define HC_HAL_I2C0_BASE      HC_NULL_PTR
#define HC_HAL_I2C0_SCL_PORT  HC_NULL_PTR
#define HC_HAL_I2C0_SCL_PIN   0u
#define HC_HAL_I2C0_SCL_IOMUX 0u
#define HC_HAL_I2C0_SDA_PORT  HC_NULL_PTR
#define HC_HAL_I2C0_SDA_PIN   0u
#define HC_HAL_I2C0_SDA_IOMUX 0u

#define HC_HAL_I2C1_ENABLE    0u
#define HC_HAL_I2C1_BASE      HC_NULL_PTR
#define HC_HAL_I2C1_SCL_PORT  HC_NULL_PTR
#define HC_HAL_I2C1_SCL_PIN   0u
#define HC_HAL_I2C1_SCL_IOMUX 0u
#define HC_HAL_I2C1_SDA_PORT  HC_NULL_PTR
#define HC_HAL_I2C1_SDA_PIN   0u
#define HC_HAL_I2C1_SDA_IOMUX 0u

/*============================================================================
 *                        WDG 模板配置
 *===========================================================================*/

/* 这些值仅用于模板默认行为；接入真实平台后按硬件约束修改。 */
#define HC_HAL_WDG_TIMEOUT_MS   2000u
#define HC_HAL_WDG_RESET_ENABLE HC_TRUE

/*============================================================================
 *                        SYSTICK 模板配置
 *===========================================================================*/

/*
 * 主模板默认维持 1ms 节拍语义，确保接口文档与超时语义稳定。
 * 若后续需要改为其他频率，请同步调整 hc_hal_systick.c 的时间换算逻辑。
 */
#define HC_HAL_SYSTICK_TICK_HZ 1000u

/*
 * 默认 CPU 主频仅用于 DelayUs 模板中的粗略忙等换算。
 * 具体移植时应替换成目标平台的真实主频值或系统时钟变量。
 */
#define HC_HAL_SYSTICK_CPU_HZ 48000000u

/* 裸机忙等模板的每次空循环开销估算值。 */
#define HC_HAL_SYSTICK_DELAY_US_CYCLES 3u

#endif /* HC_HAL_BOARD_CFG_H */
