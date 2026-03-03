# MSPM0 参考实现

本目录是基于 TI MSPM0 / DriverLib 的 HAL 参考实现目录。

## 依赖

- `hc_common_header/include`
- `hal/inc`
- `hal/example/mspm0/cfg`
- TI MSPM0 SDK 头文件路径

## 构建方式

编译时只选择本目录下的 `src/*.c`，不要同时编译：

- `hal/src/*.c`
- `hal/example/stm32/src/*.c`

## 说明

- GPIO / UART / I2C / SYSTICK / DWT / WDG 六个外设模块均已提供完整的 DriverLib 落地实现。
- DWT 模块基于 SysTick->VAL 实现（Cortex-M0+ 无 DWT CYCCNT），需先初始化 SysTick。
- 若需要接入真实项目，优先修改 `cfg/hc_hal_example_mspm0_board_cfg.h`，再按板级资源调整引脚与时钟宏。
