# HC 团队 HAL 层设计规范

当前目录已重新划分为两层：

- `hal/inc + hal/src + hal/cfg`：通用 HAL 模板层
- `hal/example`：按平台拆分的参考实现案例

## 当前定位

### 1. 主 HAL 模板层

主层的目标是：

- 提供稳定的公共接口
- 保留参数校验、状态管理和错误码语义
- 不直接绑定任何厂商 SDK
- 默认以 `TODO` / `HC_ERR_NOT_READY` 形式提示移植点

因此，主模板层：

- 不再包含 `ti_dl_lib.h`
- 不再直接使用 TI / STM32 的寄存器类型和实例宏
- 默认不启用任何 UART / I2C 实例

### 2. 平台参考实现层

`hal/example` 用于提供完整的平台目录结构和独立板级配置：

- `hal/example/mspm0`：当前 MSPM0 / DriverLib 参考实现
- `hal/example/stm32`：STM32 HAL 风格参考实现

这些案例与主 HAL 使用同一套公共头文件，但各自拥有独立的配置头和源文件目录。

## 目录结构

```text
hal/
|-- README.MD
|-- cfg/
|   `-- hc_hal_board_cfg.h
|-- inc/
|   |-- hc_hal_gpio.h
|   |-- hc_hal_uart.h
|   |-- hc_hal_i2c.h
|   |-- hc_hal_systick.h
|   |-- hc_hal_dwt.h
|   `-- hc_hal_wdg.h
|-- src/
|   |-- hc_hal_gpio.c
|   |-- hc_hal_uart.c
|   |-- hc_hal_i2c.c
|   |-- hc_hal_systick.c
|   |-- hc_hal_dwt.c
|   `-- hc_hal_wdg.c
`-- example/
    |-- README.MD
    |-- mspm0/
    |   |-- README.MD
    |   |-- cfg/
    |   |   `-- hc_hal_example_mspm0_board_cfg.h
    |   `-- src/
    |       |-- hc_hal_example_mspm0_gpio.c
    |       |-- hc_hal_example_mspm0_uart.c
    |       |-- hc_hal_example_mspm0_i2c.c
    |       |-- hc_hal_example_mspm0_systick.c
    |       `-- hc_hal_example_mspm0_wdg.c
    `-- stm32/
        |-- README.MD
        |-- inc/
        |   `-- stm32xxxx_hal.h
        |-- cfg/
        |   `-- hc_hal_example_stm32_board_cfg.h
        `-- src/
            |-- hc_hal_example_stm32_gpio.c
            |-- hc_hal_example_stm32_uart.c
            |-- hc_hal_example_stm32_i2c.c
            |-- hc_hal_example_stm32_systick.c
            |-- hc_hal_example_stm32_dwt.c
            `-- hc_hal_example_stm32_wdg.c
```

## Include 路径建议

### 主 HAL 模板层

- `hc_common_header/include`
- `hal/inc`
- `hal/cfg`

### MSPM0 案例

- `hc_common_header/include`
- `hal/inc`
- `hal/example/mspm0/cfg`
- MSPM0 SDK 头文件路径

### STM32 案例

- `hc_common_header/include`
- `hal/inc`
- `hal/example/stm32/inc`
- `hal/example/stm32/cfg`
- STM32 HAL / CMSIS 头文件路径

## 核心规则

### 1. 公共接口规则

- 公共接口统一放在 `hal/inc/*.h`
- 公共类型、错误码和函数签名尽量稳定
- 公共头文件中禁止暴露厂商 SDK 类型

### 2. 模板实现规则

- `hal/src/*.c` 只保留通用逻辑骨架
- 所有未接入的硬件路径统一返回 `HC_ERR_NOT_READY`
- 移植时优先替换 `*_hw_*` 或等效的底层接入点

### 3. 平台案例规则

- 每个平台案例必须使用独立配置头文件名
- 平台案例可以直接包含厂商头文件
- 平台案例使用真实厂商 API 风格，作为完整参考实现目录
- 平台案例不应回流污染主模板层

### 4. 配置头命名规则

- 主模板层固定使用 `hal/cfg/hc_hal_board_cfg.h`
- MSPM0 案例使用 `hc_hal_example_mspm0_board_cfg.h`
- STM32 案例使用 `hc_hal_example_stm32_board_cfg.h`

这样做的目的，是彻底避免多个 `hc_hal_board_cfg.h` 同名文件同时落入 include 搜索路径时发生冲突。

## 移植建议

当你要接入新的平台时，推荐顺序如下：

1. 先保持 `hal/inc` 不变。
2. 复制 `hal/cfg/hc_hal_board_cfg.h` 并填充目标平台配置。
3. 在 `hal/src` 中实现具体硬件接入点，逐步替换 `HC_ERR_NOT_READY`。
4. 若需要保留一套可对照的完整平台实现，再按 `hal/example/<platform>` 的结构新增案例目录。

## RTOS 说明

当前主模板仍以裸机语义为主。若迁移到 RTOS，请至少补充：

- 总线互斥
- 基于 OS Tick 的超时
- 中断到任务的事件投递
- 非忙等延时
