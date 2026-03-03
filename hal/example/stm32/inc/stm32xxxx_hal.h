/**
 * @file    stm32xxxx_hal.h
 * @brief   STM32 系列统一 HAL 头文件桥接
 *
 * 本文件根据板级配置宏 HC_HAL_EXAMPLE_STM32_DEVICE 自动选择正确的
 * STM32 HAL 头文件，消除例程中的占位头名编译错误。
 *
 * 使用方法：在 hc_hal_example_stm32_board_cfg.h 中定义：
 * @code
 * #define HC_HAL_EXAMPLE_STM32_DEVICE_F4
 * @endcode
 *
 * 支持的设备宏：
 * - HC_HAL_EXAMPLE_STM32_DEVICE_F0  → stm32f0xx_hal.h
 * - HC_HAL_EXAMPLE_STM32_DEVICE_F1  → stm32f1xx_hal.h
 * - HC_HAL_EXAMPLE_STM32_DEVICE_F2  → stm32f2xx_hal.h
 * - HC_HAL_EXAMPLE_STM32_DEVICE_F3  → stm32f3xx_hal.h
 * - HC_HAL_EXAMPLE_STM32_DEVICE_F4  → stm32f4xx_hal.h
 * - HC_HAL_EXAMPLE_STM32_DEVICE_F7  → stm32f7xx_hal.h
 * - HC_HAL_EXAMPLE_STM32_DEVICE_G0  → stm32g0xx_hal.h
 * - HC_HAL_EXAMPLE_STM32_DEVICE_G4  → stm32g4xx_hal.h
 * - HC_HAL_EXAMPLE_STM32_DEVICE_H7  → stm32h7xx_hal.h
 * - HC_HAL_EXAMPLE_STM32_DEVICE_L0  → stm32l0xx_hal.h
 * - HC_HAL_EXAMPLE_STM32_DEVICE_L4  → stm32l4xx_hal.h
 */

#ifndef STM32XXXX_HAL_H
#define STM32XXXX_HAL_H

#if defined(HC_HAL_EXAMPLE_STM32_DEVICE_F0)
#include "stm32f0xx_hal.h"
#elif defined(HC_HAL_EXAMPLE_STM32_DEVICE_F1)
#include "stm32f1xx_hal.h"
#elif defined(HC_HAL_EXAMPLE_STM32_DEVICE_F2)
#include "stm32f2xx_hal.h"
#elif defined(HC_HAL_EXAMPLE_STM32_DEVICE_F3)
#include "stm32f3xx_hal.h"
#elif defined(HC_HAL_EXAMPLE_STM32_DEVICE_F4)
#include "stm32f4xx_hal.h"
#elif defined(HC_HAL_EXAMPLE_STM32_DEVICE_F7)
#include "stm32f7xx_hal.h"
#elif defined(HC_HAL_EXAMPLE_STM32_DEVICE_G0)
#include "stm32g0xx_hal.h"
#elif defined(HC_HAL_EXAMPLE_STM32_DEVICE_G4)
#include "stm32g4xx_hal.h"
#elif defined(HC_HAL_EXAMPLE_STM32_DEVICE_H7)
#include "stm32h7xx_hal.h"
#elif defined(HC_HAL_EXAMPLE_STM32_DEVICE_L0)
#include "stm32l0xx_hal.h"
#elif defined(HC_HAL_EXAMPLE_STM32_DEVICE_L4)
#include "stm32l4xx_hal.h"
#else
#error "Please define HC_HAL_EXAMPLE_STM32_DEVICE_Fx in board_cfg.h (e.g. HC_HAL_EXAMPLE_STM32_DEVICE_F4)"
#endif

#endif /* STM32XXXX_HAL_H */
