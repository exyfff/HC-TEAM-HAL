# STM32 参考实现

本目录是基于 STM32 HAL / CMSIS 风格的 HAL 参考实现目录。

## 依赖

- `hc_common_header/include`
- `hal/inc`
- `hal/example/stm32/inc`（桥接头 `stm32xxxx_hal.h`）
- `hal/example/stm32/cfg`
- STM32 HAL / CMSIS 头文件路径

## 构建方式

编译时只选择本目录下的 `src/*.c`，不要同时编译：

- `hal/src/*.c`
- `hal/example/mspm0/src/*.c`

## 说明

- `inc/stm32xxxx_hal.h` 是条件编译桥接头，根据 `board_cfg.h` 中定义的设备宏自动包含正确的 STM32 HAL 头文件。
- GPIO / UART / I2C / SYSTICK / DWT / WDG 六个外设模块均已提供完整的 STM32 HAL 落地实现。
- DWT 模块基于 Cortex-M3/M4/M7 的 `DWT->CYCCNT` 实现精确微秒延迟。
- 若切换到具体芯片，优先修改 `cfg/hc_hal_example_stm32_board_cfg.h` 中的设备宏、实例、引脚和 RCC 宏。
