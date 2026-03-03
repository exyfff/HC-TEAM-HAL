/**
 * @file    hc_hal_example_stm32_board_cfg.h
 * @brief   HC 团队 HAL STM32 参考实现板级配置
 *
 * 本文件服务于 `hal/example/stm32` 下的参考实现。
 * 它使用标准 STM32 HAL 风格的实例、引脚和 RCC/NVIC 宏。
 * 若切换到具体芯片，请优先修改本文件中的宏，而不是直接改动源文件。
 */

#ifndef HC_HAL_EXAMPLE_STM32_BOARD_CFG_H
#define HC_HAL_EXAMPLE_STM32_BOARD_CFG_H

/*============================================================================
 *                        STM32 目标芯片系列选择
 *
 * 取消注释对应的宏以选择 STM32 系列，stm32xxxx_hal.h 桥接头将自动包含
 * 正确的 HAL 头文件。同一时间只能启用一个。
 *===========================================================================*/
/* #define HC_HAL_EXAMPLE_STM32_DEVICE_F1 */
/* #define HC_HAL_EXAMPLE_STM32_DEVICE_F3 */
#define HC_HAL_EXAMPLE_STM32_DEVICE_F4
/* #define HC_HAL_EXAMPLE_STM32_DEVICE_F7 */
/* #define HC_HAL_EXAMPLE_STM32_DEVICE_G4 */
/* #define HC_HAL_EXAMPLE_STM32_DEVICE_H7 */
/* #define HC_HAL_EXAMPLE_STM32_DEVICE_L4 */

#include "hc_common.h"
#include "stm32xxxx_hal.h"

/*============================================================================
 *                        GPIO 时钟与虚拟引脚映射
 *===========================================================================*/

#define HC_HAL_EXAMPLE_STM32_GPIOA_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define HC_HAL_EXAMPLE_STM32_GPIOB_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define HC_HAL_EXAMPLE_STM32_GPIOC_CLK_ENABLE() __HAL_RCC_GPIOC_CLK_ENABLE()

/* STM32 参考实现的虚拟引脚号枚举 */
typedef enum {
    VPIN_LED_1 = 0,      /* LED1 */
    VPIN_LED_2 = 1,      /* LED2 */
    VPIN_LED_3 = 2,      /* LED3 */
    VPIN_RELAY_1 = 3,    /* 继电器输出 1 */
    VPIN_RELAY_2 = 4,    /* 继电器输出 2 */
    VPIN_BUZZER_EN = 5,  /* 蜂鸣器使能 */
    VPIN_MOTOR_EN = 6,   /* 电机使能 */
    VPIN_SENSOR_PWR = 7, /* 传感器电源使能 */
    VPIN_SPI_CS_0 = 8,   /* SPI 片选 0 */
    VPIN_KEY_1 = 9,      /* 按键输入 1 */
    VPIN_KEY_2 = 10,     /* 按键输入 2 */
    VPIN_EXTI_1 = 11,    /* 外部中断输入 1 */
    VPIN_MAX
} HC_HAL_GPIO_VPin_e;

/*
 * GPIO 中断向量名: STM32 使用 EXTI 线级中断。
 * 根据实际按键/外部中断分配的 EXTI 线选择对应的 IRQn Handler。
 * 此处 GPIOA 映射至 EXTI15_10（KEY_1 使用 PC13）；
 * GPIOB 映射至 EXTI9_5（KEY_2 使用 PA8）。
 */
#define HC_HAL_GPIOA_IRQHandler EXTI15_10_IRQHandler
#define HC_HAL_GPIOB_IRQHandler EXTI9_5_IRQHandler

#define HC_HAL_EXAMPLE_STM32_VPIN_LED_1_PORT      GPIOA
#define HC_HAL_EXAMPLE_STM32_VPIN_LED_1_PIN       GPIO_PIN_0
#define HC_HAL_EXAMPLE_STM32_VPIN_LED_2_PORT      GPIOA
#define HC_HAL_EXAMPLE_STM32_VPIN_LED_2_PIN       GPIO_PIN_1
#define HC_HAL_EXAMPLE_STM32_VPIN_LED_3_PORT      GPIOA
#define HC_HAL_EXAMPLE_STM32_VPIN_LED_3_PIN       GPIO_PIN_2
#define HC_HAL_EXAMPLE_STM32_VPIN_RELAY_1_PORT    GPIOB
#define HC_HAL_EXAMPLE_STM32_VPIN_RELAY_1_PIN     GPIO_PIN_0
#define HC_HAL_EXAMPLE_STM32_VPIN_RELAY_2_PORT    GPIOB
#define HC_HAL_EXAMPLE_STM32_VPIN_RELAY_2_PIN     GPIO_PIN_1
#define HC_HAL_EXAMPLE_STM32_VPIN_BUZZER_EN_PORT  GPIOB
#define HC_HAL_EXAMPLE_STM32_VPIN_BUZZER_EN_PIN   GPIO_PIN_2
#define HC_HAL_EXAMPLE_STM32_VPIN_MOTOR_EN_PORT   GPIOB
#define HC_HAL_EXAMPLE_STM32_VPIN_MOTOR_EN_PIN    GPIO_PIN_10
#define HC_HAL_EXAMPLE_STM32_VPIN_SENSOR_PWR_PORT GPIOB
#define HC_HAL_EXAMPLE_STM32_VPIN_SENSOR_PWR_PIN  GPIO_PIN_12
#define HC_HAL_EXAMPLE_STM32_VPIN_SPI_CS_0_PORT   GPIOB
#define HC_HAL_EXAMPLE_STM32_VPIN_SPI_CS_0_PIN    GPIO_PIN_13
#define HC_HAL_EXAMPLE_STM32_VPIN_KEY_1_PORT      GPIOC
#define HC_HAL_EXAMPLE_STM32_VPIN_KEY_1_PIN       GPIO_PIN_13
#define HC_HAL_EXAMPLE_STM32_VPIN_KEY_1_IRQn      EXTI15_10_IRQn
#define HC_HAL_EXAMPLE_STM32_VPIN_KEY_2_PORT      GPIOA
#define HC_HAL_EXAMPLE_STM32_VPIN_KEY_2_PIN       GPIO_PIN_8
#define HC_HAL_EXAMPLE_STM32_VPIN_KEY_2_IRQn      EXTI9_5_IRQn
#define HC_HAL_EXAMPLE_STM32_VPIN_EXTI_1_PORT     GPIOB
#define HC_HAL_EXAMPLE_STM32_VPIN_EXTI_1_PIN      GPIO_PIN_14
#define HC_HAL_EXAMPLE_STM32_VPIN_EXTI_1_IRQn     EXTI15_10_IRQn

/*============================================================================
 *                        UART 板级资源分配
 *===========================================================================*/

/* UART 中断向量名: STM32 使用 USARTx_IRQHandler */
#define HC_HAL_UART0_IRQHandler USART1_IRQHandler
#define HC_HAL_UART1_IRQHandler USART2_IRQHandler

#define HC_HAL_EXAMPLE_STM32_UART0_ENABLE       1u
#define HC_HAL_EXAMPLE_STM32_UART0_INSTANCE     USART1
#define HC_HAL_EXAMPLE_STM32_UART0_CLK_ENABLE() __HAL_RCC_USART1_CLK_ENABLE()
#define HC_HAL_EXAMPLE_STM32_UART0_TX_PORT      GPIOA
#define HC_HAL_EXAMPLE_STM32_UART0_TX_PIN       GPIO_PIN_9
#define HC_HAL_EXAMPLE_STM32_UART0_TX_AF        GPIO_AF7_USART1
#define HC_HAL_EXAMPLE_STM32_UART0_RX_PORT      GPIOA
#define HC_HAL_EXAMPLE_STM32_UART0_RX_PIN       GPIO_PIN_10
#define HC_HAL_EXAMPLE_STM32_UART0_RX_AF        GPIO_AF7_USART1
#define HC_HAL_EXAMPLE_STM32_UART0_IRQn         USART1_IRQn

#define HC_HAL_EXAMPLE_STM32_UART1_ENABLE       0u
#define HC_HAL_EXAMPLE_STM32_UART1_INSTANCE     USART2
#define HC_HAL_EXAMPLE_STM32_UART1_CLK_ENABLE() __HAL_RCC_USART2_CLK_ENABLE()
#define HC_HAL_EXAMPLE_STM32_UART1_TX_PORT      GPIOA
#define HC_HAL_EXAMPLE_STM32_UART1_TX_PIN       GPIO_PIN_2
#define HC_HAL_EXAMPLE_STM32_UART1_TX_AF        GPIO_AF7_USART2
#define HC_HAL_EXAMPLE_STM32_UART1_RX_PORT      GPIOA
#define HC_HAL_EXAMPLE_STM32_UART1_RX_PIN       GPIO_PIN_3
#define HC_HAL_EXAMPLE_STM32_UART1_RX_AF        GPIO_AF7_USART2
#define HC_HAL_EXAMPLE_STM32_UART1_IRQn         USART2_IRQn

/*============================================================================
 *                        I2C 板级资源分配
 *===========================================================================*/

#define HC_HAL_EXAMPLE_STM32_I2C0_ENABLE       1u
#define HC_HAL_EXAMPLE_STM32_I2C0_INSTANCE     I2C1
#define HC_HAL_EXAMPLE_STM32_I2C0_CLK_ENABLE() __HAL_RCC_I2C1_CLK_ENABLE()
#define HC_HAL_EXAMPLE_STM32_I2C0_SCL_PORT     GPIOB
#define HC_HAL_EXAMPLE_STM32_I2C0_SCL_PIN      GPIO_PIN_8
#define HC_HAL_EXAMPLE_STM32_I2C0_SCL_AF       GPIO_AF4_I2C1
#define HC_HAL_EXAMPLE_STM32_I2C0_SDA_PORT     GPIOB
#define HC_HAL_EXAMPLE_STM32_I2C0_SDA_PIN      GPIO_PIN_9
#define HC_HAL_EXAMPLE_STM32_I2C0_SDA_AF       GPIO_AF4_I2C1
/*
 * I2C Timing 值必须由 STM32CubeMX "I2C Configuration → Timing" 计算器生成。
 * 此值为寄存器级编码，与 bus_khz 无直接数学关系，不可手动推算。
 * 典型参考 (48MHz PCLK):
 *   100 kHz (Standard Mode) : 0x10909CECul
 *   400 kHz (Fast Mode)     : 0x00702991ul
 * 仅在定义了 I2C_TIMINGR_PRESC 的系列 (G0/L0/H7 等) 中使用；
 * F1/F4 系列使用 ClockSpeed/DutyCycle，此宏不生效。
 */
#define HC_HAL_EXAMPLE_STM32_I2C0_TIMING      0x10909CECul
#define HC_HAL_EXAMPLE_STM32_I2C0_TIMING_400K 0x00702991ul

#define HC_HAL_EXAMPLE_STM32_I2C1_ENABLE       0u
#define HC_HAL_EXAMPLE_STM32_I2C1_INSTANCE     I2C2
#define HC_HAL_EXAMPLE_STM32_I2C1_CLK_ENABLE() __HAL_RCC_I2C2_CLK_ENABLE()
#define HC_HAL_EXAMPLE_STM32_I2C1_SCL_PORT     GPIOB
#define HC_HAL_EXAMPLE_STM32_I2C1_SCL_PIN      GPIO_PIN_10
#define HC_HAL_EXAMPLE_STM32_I2C1_SCL_AF       GPIO_AF4_I2C2
#define HC_HAL_EXAMPLE_STM32_I2C1_SDA_PORT     GPIOB
#define HC_HAL_EXAMPLE_STM32_I2C1_SDA_PIN      GPIO_PIN_11
#define HC_HAL_EXAMPLE_STM32_I2C1_SDA_AF       GPIO_AF4_I2C2
#define HC_HAL_EXAMPLE_STM32_I2C1_TIMING       0x10909CECul
#define HC_HAL_EXAMPLE_STM32_I2C1_TIMING_400K  0x00702991ul

/*============================================================================
 *                        WDG / SYSTICK 板级资源分配
 *===========================================================================*/

#define HC_HAL_EXAMPLE_STM32_WDG_PRESCALER   IWDG_PRESCALER_64
#define HC_HAL_EXAMPLE_STM32_WDG_RELOAD      1250u
#define HC_HAL_EXAMPLE_STM32_SYSTICK_TICK_HZ 1000u

#endif /* HC_HAL_EXAMPLE_STM32_BOARD_CFG_H */
