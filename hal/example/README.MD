# HAL 平台案例总览

本目录用于提供与主 HAL 公共接口配套的平台参考实现。

与主模板层不同，这里的代码：

- 按平台拆目录
- 使用独立的板级配置头
- 允许直接依赖厂商 SDK / HAL
- 目标是提供可落地的实现案例，而不是纯注释模板

## 当前案例

### `mspm0`

- 面向 TI MSPM0 / DriverLib 风格
- 复用 `hal/inc` 公共接口
- 配置头：`cfg/hc_hal_example_mspm0_board_cfg.h`

### `stm32`

- 面向 STM32 HAL / CMSIS 风格
- 复用 `hal/inc` 公共接口
- 配置头：`cfg/hc_hal_example_stm32_board_cfg.h`

## 使用方式

1. 选择一个平台目录作为“活动实现”。
2. 将对应平台目录的 `cfg` 加入 include 路径。
3. 仅编译该平台目录下的 `src/*.c`，不要与 `hal/src/*.c` 同时编译。
4. 同时加入对应厂商 SDK / HAL 的头文件路径与库。

## 注意事项

- `mspm0` 与 `stm32` 都实现了与主 HAL 相同的公共函数名。
- 这些案例用于“替代主模板层进入具体工程”，而不是与主模板层一起链接。
- 若新增第三个平台，继续沿用 `example/<platform>/cfg + src` 的结构即可。
