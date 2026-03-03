/**
 * @file    hc_hal_example_mspm0_board_cfg.h
 * @brief   HC 团队 HAL MSPM0 参考实现板级配置
 *
 * 本文件服务于 `hal/example/mspm0` 下的完整参考实现。
 * 它绑定的是当前活动的 MSPM0 / DriverLib 参考资源，不属于主 HAL 模板层。
 */

#ifndef HC_HAL_EXAMPLE_MSPM0_BOARD_CFG_H
#define HC_HAL_EXAMPLE_MSPM0_BOARD_CFG_H

#include "hc_common.h"
#include "ti_dl_lib.h"

/*============================================================================
 *                        GPIO 板级资源分配
 *===========================================================================*/

#define HC_HAL_GPIO_PORT_NUM 2u
#define HC_HAL_GPIO_PIN_NUM  32u

/* MSPM0 参考实现的虚拟引脚号枚举 */
typedef enum {
    VPIN_LED_1 = 0,
    VPIN_LED_2 = 1,
    VPIN_LED_3 = 2,
    VPIN_RELAY_1 = 3,
    VPIN_RELAY_2 = 4,
    VPIN_BUZZER_EN = 5,
    VPIN_MOTOR_EN = 6,
    VPIN_SENSOR_PWR = 7,
    VPIN_SPI_CS_0 = 8,
    VPIN_KEY_1 = 9,
    VPIN_KEY_2 = 10,
    VPIN_EXTI_1 = 11,
    VPIN_MAX
} HC_HAL_GPIO_VPin_e;

/* GPIO 中断向量名: MSPM0 使用端口级中断 */
#define HC_HAL_GPIOA_IRQHandler GPIOA_IRQHandler
#define HC_HAL_GPIOB_IRQHandler GPIOB_IRQHandler

/*============================================================================
 *                        UART 板级资源分配
 *===========================================================================*/

/* UART 中断向量名: MSPM0 使用外设级中断 */
#define HC_HAL_UART0_IRQHandler UART0_INT_IRQHandler
#define HC_HAL_UART1_IRQHandler UART1_INT_IRQHandler

#define HC_HAL_UART0_ENABLE   1u
#define HC_HAL_UART0_BASE     UART0
#define HC_HAL_UART0_TX_PORT  GPIOA
#define HC_HAL_UART0_TX_PIN   DL_GPIO_PIN_10
#define HC_HAL_UART0_TX_IOMUX IOMUX_PA10_UART0_TX
#define HC_HAL_UART0_RX_PORT  GPIOA
#define HC_HAL_UART0_RX_PIN   DL_GPIO_PIN_11
#define HC_HAL_UART0_RX_IOMUX IOMUX_PA11_UART0_RX
#define HC_HAL_UART0_IRQn     UART0_INT_IRQn
#define HC_HAL_UART0_IRQ_PRIO 5u

#define HC_HAL_UART1_ENABLE   0u
#define HC_HAL_UART1_BASE     UART1
#define HC_HAL_UART1_TX_PORT  GPIOB
#define HC_HAL_UART1_TX_PIN   DL_GPIO_PIN_0
#define HC_HAL_UART1_TX_IOMUX IOMUX_PB0_UART1_TX
#define HC_HAL_UART1_RX_PORT  GPIOB
#define HC_HAL_UART1_RX_PIN   DL_GPIO_PIN_1
#define HC_HAL_UART1_RX_IOMUX IOMUX_PB1_UART1_RX
#define HC_HAL_UART1_IRQn     UART1_INT_IRQn
#define HC_HAL_UART1_IRQ_PRIO 5u

/*============================================================================
 *                        I2C 板级资源分配
 *===========================================================================*/

#define HC_HAL_I2C0_ENABLE    1u
#define HC_HAL_I2C0_BASE      I2C0
#define HC_HAL_I2C0_SCL_PORT  GPIOA
#define HC_HAL_I2C0_SCL_PIN   DL_GPIO_PIN_6
#define HC_HAL_I2C0_SCL_IOMUX IOMUX_PA6_I2C0_SCL
#define HC_HAL_I2C0_SDA_PORT  GPIOA
#define HC_HAL_I2C0_SDA_PIN   DL_GPIO_PIN_7
#define HC_HAL_I2C0_SDA_IOMUX IOMUX_PA7_I2C0_SDA

#define HC_HAL_I2C1_ENABLE    0u
#define HC_HAL_I2C1_BASE      I2C1
#define HC_HAL_I2C1_SCL_PORT  GPIOB
#define HC_HAL_I2C1_SCL_PIN   DL_GPIO_PIN_6
#define HC_HAL_I2C1_SCL_IOMUX IOMUX_PB6_I2C1_SCL
#define HC_HAL_I2C1_SDA_PORT  GPIOB
#define HC_HAL_I2C1_SDA_PIN   DL_GPIO_PIN_7
#define HC_HAL_I2C1_SDA_IOMUX IOMUX_PB7_I2C1_SDA

/*============================================================================
 *                        WDG 板级资源分配
 *===========================================================================*/

#define HC_HAL_WDG_TIMEOUT_MS   2000u
#define HC_HAL_WDG_RESET_ENABLE HC_TRUE
/* 32.768 kHz LFCLK, divide-by-2 + 2^15 count, 实际约 2s 看门狗窗口 */
#define HC_HAL_EXAMPLE_MSPM0_WDG_BASE          LFSS
#define HC_HAL_EXAMPLE_MSPM0_WDG_CLOCK_DIVIDER DL_IWDT_CLOCK_DIVIDE_2
#define HC_HAL_EXAMPLE_MSPM0_WDG_TIMER_PERIOD  DL_IWDT_TIMER_PERIOD_15_BITS

/*============================================================================
 *                        SYSTICK 板级资源分配
 *===========================================================================*/

#define HC_HAL_EXAMPLE_MSPM0_PERIPH_CLK_HZ SystemCoreClock
#define HC_HAL_SYSTICK_TICK_HZ             1000u
#define HC_HAL_SYSTICK_CPU_HZ              SystemCoreClock
#define HC_HAL_SYSTICK_DELAY_US_CYCLES     3u

#endif /* HC_HAL_EXAMPLE_MSPM0_BOARD_CFG_H */
