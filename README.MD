# HC 团队 HAL 设计示例仓库

本仓库用于沉淀 HC 团队的 HAL 设计方法，当前定位为：

- 通用接口模板（实现允许平台耦合）
- 尽量快速可移植的通用 HAL
- 面向阅读、对照和后续项目落地的设计示例

需要特别说明的是：

- `hal/inc` 中的公共接口是主要稳定面，尽量不随平台变化而变化。
- `hal/src` 中的当前实现允许保留平台耦合，但应尽量把平台调用集中到少量私有 helper 中。
- `hal/example` 里的内容仅供阅读和理解使用，不参与主构建，也不作为后续正式移植输入。

## 目录总览

```text
.vscode/                        VSCode 工作区配置（C/C++ 插件、格式化、性能优化）
hc_common_header/include/       基础类型、错误码、工具宏（通用公共头）
hal/cfg/                        当前活动参考平台的板级资源绑定
hal/inc/                        通用 HAL 公共接口
hal/src/                        当前参考实现（允许平台耦合）
hal/example/                    阅读型参考示例（仅供查看）
```

## 通用公共头 (`hc_common_header`)

所有 HAL 模块和平台案例的基础依赖，包含四个核心头文件：

| 文件 | 职责 |
|------|------|
| `hc_type.h` | 基础类型别名（`HC_U8` / `HC_U16` / `HC_U32` / `HC_S32` / `HC_VOID` 等） |
| `hc_error.h` | 统一错误码定义（`HC_HAL_OK` / `HC_HAL_ERR_TIMEOUT` / `HC_HAL_ERR_NACK` 等） |
| `hc_macro.h` | 工具宏集合（`HC_ARRAY_SIZE` / `HC_SWAP` / `HC_BitSet` / `HC_UNUSED` 等） |
| `hc_common.h` | 总入口头，汇总包含以上三个文件 |

任何 HAL 源文件只需 `#include "hc_common.h"` 即可获取全部公共定义。

## VSCode C/C++ 插件配置

本仓库附带 `.vscode/` 配置，开箱即用：

### `settings.json` 核心配置

- **编码统一**：UTF-8 + LF 换行
- **格式化**：保存自动格式化，使用 Clang-Format（格式规则 `.vscode/.clang-format`）
- **文件关联**：`.h` / `.c` 强制关联为 C 语言
- **性能优化**：
  - `C_Cpp.workspaceParsingPriority: low` — 防止解析阻塞 UI
  - `C_Cpp.intelliSenseCacheSize: 2048` — 限制缓存防内存爆炸
  - `files.watcherExclude` — 排除 `.git` / `build` / `output` 减少文件监视压力
  - `C_Cpp.files.exclude` — 排除编译产物，避免跳转污染

### `c_cpp_properties.json` 三套配置

| 配置名 | 用途 | include 路径 |
|--------|------|-------------|
| `HAL-Template` | 主模板层开发 | `hc_common_header` + `hal/inc` + `hal/cfg` |
| `Example-MSPM0` | MSPM0 案例阅读 | 上述 + `hal/example/mspm0/cfg` + `${MSPM0_SDK_PATH}` |
| `Example-STM32` | STM32 案例阅读 | 上述 + `hal/example/stm32/cfg` + `hal/example/stm32/inc` + `${STM32_HAL_PATH}` |

使用时在 VSCode 右下角状态栏切换活动配置即可。

## 模块覆盖

本仓库覆盖以下 6 个外设模块，每个模块均提供公共接口 + 模板骨架 + 双平台参考实现：

| 模块 | 公共接口 | 关键特性 |
|------|----------|----------|
| GPIO | `hc_hal_gpio.h` | 虚拟引脚抽象 + EXTI 中断转发 + 回调通知 |
| UART | `hc_hal_uart.h` | 句柄式多实例 + 阻塞发送 + 中断接收回调 |
| I2C | `hc_hal_i2c.h` | 句柄式多实例 + Repeated-START 寄存器访问 + 超时保护 |
| SysTick | `hc_hal_systick.h` | 1ms 时基 + 阻塞延时（ms / µs） |
| DWT | `hc_hal_dwt.h` | 硬件精确微秒延迟（M3/M4 CYCCNT，M0+ SysTick 替代） |
| WDG | `hc_hal_wdg.h` | 独立看门狗 + 冷启动复位来源锁定 |

## 阅读顺序建议

1. 先看 `hal/README.MD`，理解主规则和边界。
2. 再看 `hc_common_header/include/hc_common.h`，了解基础类型和错误码体系。
3. 再看 `hal/inc/*.h`，了解稳定的公共接口长什么样。
4. 再看 `hal/src/*.c`，理解当前参考实现如何组织状态、参数校验和 `*_hw_*` 私有 helper。
5. 最后看 `hal/example/` 下的平台案例，对照不同厂商 API 的写法差异。

## 当前边界

- 本仓库不是现成量产驱动库。
- 本仓库不是"零修改跨平台"的完整 HAL 框架。
- 本仓库强调的是：公共接口先稳定，平台差异再被约束在可控边界内。

## 快速移植的含义

本仓库中的"尽量快速可移植"并不表示"复制即可跨平台运行"，而是表示：

- 上层业务代码尽量不改；
- 公共接口尽量不改；
- 真正的移植工作尽量集中在：
  - `hal/cfg/hc_hal_board_cfg.h`
  - `hal/src/*.c` 中的 `*_hw_*` 私有 helper

`hal/example` 只负责帮助使用者看懂"换一个厂商时底层 API 大致怎么接"，不承担正式实现职责。
