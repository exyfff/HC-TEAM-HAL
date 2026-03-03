/**
 * @file    hc_hal_example_mspm0_gpio.c
 * @brief   MSPM0 平台的 HC HAL GPIO 参考实现。
 *
 * 本文件演示如何将通用 HC HAL GPIO 接口映射到 TI MSPM0 DriverLib。
 */

#include "ti_dl_lib.h"
#include "hc_hal_example_mspm0_board_cfg.h"
#include "hc_hal_gpio.h"

/*============================================================================
 *                        内部私有定义
 *===========================================================================*/

typedef enum {
    DIR_IN = 0,
    DIR_OUT = 1
} PinDir_e;

typedef enum {
    PULL_NONE = 0,
    PULL_UP = 1,
    PULL_DOWN = 2
} PinPull_e;

typedef enum {
    IRQ_NONE = 0,
    IRQ_RISING = 1,
    IRQ_FALLING = 2,
    IRQ_BOTH = 3
} PinIRQ_e;

typedef struct {
    GPIO_Regs *port;
    HC_U32 pin_mask;
    PinDir_e dir;
    PinPull_e pull;
    HC_HAL_GPIO_PinState_e init_level;
    PinIRQ_e irq;
} PinCfg_t;

#define HC_GPIO_IRQ_PORT_NUM HC_HAL_GPIO_PORT_NUM
#define HC_GPIO_IRQ_PIN_NUM  HC_HAL_GPIO_PIN_NUM
#define HC_GPIO_VPIN_INVALID 0xFFu

/*============================================================================
 *                        私有辅助函数声明
 *===========================================================================*/

static HC_VOID gpio_set_irq_polarity(const PinCfg_t *cfg);
static HC_VOID gpio_cfg_direction(const PinCfg_t *cfg);
static HC_VOID gpio_cfg_pull(const PinCfg_t *cfg);
static HC_VOID gpio_cfg_irq(const PinCfg_t *cfg);
static HC_S32 gpio_get_port_index(GPIO_Regs *port, HC_U8 *p_port_idx);
static HC_S32 gpio_pin_mask_to_bit_index(HC_U32 pin_mask, HC_U8 *p_bit_idx);
static HC_S32 gpio_build_irq_lut(HC_VOID);
static HC_Bool_e gpio_is_valid_vpin(HC_HAL_GPIO_VPin_e vpin);
static HC_S32 gpio_check_vpin(HC_HAL_GPIO_VPin_e vpin);
static HC_S32 gpio_check_output_vpin(HC_HAL_GPIO_VPin_e vpin);
static HC_VOID gpio_irq_handler(GPIO_Regs *port, HC_U8 port_idx);

/*============================================================================
 *                        私有全局变量与配置表
 *===========================================================================*/

static const PinCfg_t s_pin_map[] = {
    {GPIOA, DL_GPIO_PIN_12, DIR_OUT, PULL_NONE, HC_PIN_RESET, IRQ_NONE},
    {GPIOA, DL_GPIO_PIN_13, DIR_OUT, PULL_NONE, HC_PIN_RESET, IRQ_NONE},
    {GPIOA, DL_GPIO_PIN_14, DIR_OUT, PULL_NONE, HC_PIN_RESET, IRQ_NONE},
    {GPIOB, DL_GPIO_PIN_2, DIR_OUT, PULL_NONE, HC_PIN_RESET, IRQ_NONE},
    {GPIOB, DL_GPIO_PIN_3, DIR_OUT, PULL_NONE, HC_PIN_RESET, IRQ_NONE},
    {GPIOB, DL_GPIO_PIN_4, DIR_OUT, PULL_NONE, HC_PIN_RESET, IRQ_NONE},
    {GPIOB, DL_GPIO_PIN_8, DIR_OUT, PULL_NONE, HC_PIN_RESET, IRQ_NONE},
    {GPIOA, DL_GPIO_PIN_22, DIR_OUT, PULL_NONE, HC_PIN_RESET, IRQ_NONE},
    {GPIOA, DL_GPIO_PIN_23, DIR_OUT, PULL_NONE, HC_PIN_RESET, IRQ_NONE},
    {GPIOA, DL_GPIO_PIN_3, DIR_IN, PULL_UP, HC_PIN_RESET, IRQ_FALLING},
    {GPIOA, DL_GPIO_PIN_4, DIR_IN, PULL_UP, HC_PIN_RESET, IRQ_FALLING},
    {GPIOB, DL_GPIO_PIN_1, DIR_IN, PULL_UP, HC_PIN_RESET, IRQ_FALLING},
};

typedef char PinMapSizeCheck_t[(HC_ARRAY_SIZE(s_pin_map) == VPIN_MAX) ? 1 : -1];

static volatile HC_Bool_e s_gpio_is_init = HC_FALSE;
static HC_U8 s_gpio_irq_vpin_lut[HC_GPIO_IRQ_PORT_NUM][HC_GPIO_IRQ_PIN_NUM];

/*============================================================================
 *                        私有辅助函数实现
 *===========================================================================*/

/*
 * MSPM0 POLARITY 寄存器每引脚占 2 bit:
 *   00 = Disable, 01 = Rise, 10 = Fall, 11 = RiseFall
 * DriverLib 仅提供 setLowerPinsPolarity / setUpperPinsPolarity（|= 语义），
 * 无法清位切换方向，故此处直接 read-modify-write 寄存器。
 */
#define GPIO_POLARITY_RISE 1u
#define GPIO_POLARITY_FALL 2u

static HC_VOID gpio_set_irq_polarity(const PinCfg_t *cfg)
{
    HC_U8 bit_idx;
    HC_U32 edge_code;
    HC_U32 shift;
    HC_U32 clear_mask;
    volatile HC_U32 *p_reg;

    if (cfg->irq == IRQ_NONE) {
        return;
    }

    if (gpio_pin_mask_to_bit_index(cfg->pin_mask, &bit_idx) != HC_HAL_OK) {
        return;
    }

    switch (cfg->irq) {
        case IRQ_RISING:
            edge_code = GPIO_POLARITY_RISE;
            break;
        case IRQ_FALLING:
            edge_code = GPIO_POLARITY_FALL;
            break;
        case IRQ_BOTH:
            edge_code = (DL_GPIO_readPins(cfg->port, cfg->pin_mask) != 0u) ? GPIO_POLARITY_FALL : GPIO_POLARITY_RISE;
            break;
        default:
            return;
    }

    if (bit_idx < 16u) {
        shift = (HC_U32)bit_idx * 2u;
        p_reg = &cfg->port->POLARITY15_0;
    } else {
        shift = (HC_U32)(bit_idx - 16u) * 2u;
        p_reg = &cfg->port->POLARITY31_16;
    }

    clear_mask = (HC_U32)0x3u << shift;
    *p_reg = (*p_reg & ~clear_mask) | (edge_code << shift);
}

static HC_S32 gpio_get_port_index(GPIO_Regs *port, HC_U8 *p_port_idx)
{
    if (p_port_idx == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (port == GPIOA) {
        *p_port_idx = 0u;
        return HC_HAL_OK;
    }

    if (port == GPIOB) {
        *p_port_idx = 1u;
        return HC_HAL_OK;
    }

    return HC_HAL_ERR_INVALID;
}

static HC_S32 gpio_pin_mask_to_bit_index(HC_U32 pin_mask, HC_U8 *p_bit_idx)
{
    if (p_bit_idx == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (pin_mask == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    if ((pin_mask & (pin_mask - 1u)) != 0u) {
        return HC_HAL_ERR_INVALID;
    }

#if defined(__GNUC__) || defined(__clang__)
    *p_bit_idx = (HC_U8)__builtin_ctz(pin_mask);
#else
    {
        HC_U8 bit_idx = 0u;

        while (((pin_mask >> bit_idx) & 0x1u) == 0u) {
            bit_idx++;
        }

        *p_bit_idx = bit_idx;
    }
#endif

    if (*p_bit_idx >= HC_GPIO_IRQ_PIN_NUM) {
        return HC_HAL_ERR_INVALID;
    }

    return HC_HAL_OK;
}

static HC_S32 gpio_build_irq_lut(HC_VOID)
{
    HC_U8 port_idx;
    HC_U8 bit_idx;
    HC_U8 i;
    HC_U8 j;
    HC_S32 ret;

    for (i = 0u; i < HC_GPIO_IRQ_PORT_NUM; i++) {
        for (j = 0u; j < HC_GPIO_IRQ_PIN_NUM; j++) {
            s_gpio_irq_vpin_lut[i][j] = HC_GPIO_VPIN_INVALID;
        }
    }

    for (i = 0u; i < (HC_U8)HC_ARRAY_SIZE(s_pin_map); i++) {
        if (s_pin_map[i].irq == IRQ_NONE) {
            continue;
        }

        ret = gpio_get_port_index(s_pin_map[i].port, &port_idx);
        if (ret != HC_HAL_OK) {
            return ret;
        }

        ret = gpio_pin_mask_to_bit_index(s_pin_map[i].pin_mask, &bit_idx);
        if (ret != HC_HAL_OK) {
            return ret;
        }

        if (s_gpio_irq_vpin_lut[port_idx][bit_idx] != HC_GPIO_VPIN_INVALID) {
            return HC_HAL_ERR_INVALID;
        }

        s_gpio_irq_vpin_lut[port_idx][bit_idx] = i;
    }

    return HC_HAL_OK;
}

static HC_Bool_e gpio_is_valid_vpin(HC_HAL_GPIO_VPin_e vpin)
{
    return (((HC_U32)vpin) < (HC_U32)HC_ARRAY_SIZE(s_pin_map)) ? HC_TRUE : HC_FALSE;
}

static HC_S32 gpio_check_vpin(HC_HAL_GPIO_VPin_e vpin)
{
    if (gpio_is_valid_vpin(vpin) == HC_FALSE) {
        return HC_HAL_ERR_INVALID;
    }

    if (s_gpio_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    return HC_HAL_OK;
}

static HC_S32 gpio_check_output_vpin(HC_HAL_GPIO_VPin_e vpin)
{
    HC_S32 ret = gpio_check_vpin(vpin);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    if (s_pin_map[vpin].dir != DIR_OUT) {
        return HC_HAL_ERR_NOT_PERM;
    }

    return HC_HAL_OK;
}

static HC_VOID gpio_cfg_direction(const PinCfg_t *cfg)
{
    if (cfg->dir == DIR_OUT) {
        if (cfg->init_level == HC_PIN_SET) {
            DL_GPIO_setPins(cfg->port, cfg->pin_mask);
        } else {
            DL_GPIO_clearPins(cfg->port, cfg->pin_mask);
        }

        DL_GPIO_enableOutput(cfg->port, cfg->pin_mask);
    } else {
        DL_GPIO_disableOutput(cfg->port, cfg->pin_mask);
    }
}

static HC_VOID gpio_cfg_pull(const PinCfg_t *cfg)
{
    DL_GPIO_RESISTOR resistor;

    /* 仅对输入引脚配置上下拉；输出引脚由驱动器控制，无需 pullup/pulldown */
    if (cfg->dir != DIR_IN) {
        return;
    }

    switch (cfg->pull) {
        case PULL_UP:
            resistor = DL_GPIO_RESISTOR_PULL_UP;
            break;
        case PULL_DOWN:
            resistor = DL_GPIO_RESISTOR_PULL_DOWN;
            break;
        case PULL_NONE:
        default:
            resistor = DL_GPIO_RESISTOR_NONE;
            break;
    }

    /*
     * DL_GPIO_initDigitalInput 配置 IOMUX PAD 寄存器的 PIPU/PIPD 位，
     * 同时设置迟滞和唤醒属性。
     */
    DL_GPIO_initDigitalInput(cfg->port, cfg->pin_mask, resistor, DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
}

static HC_VOID gpio_cfg_irq(const PinCfg_t *cfg)
{
    if (cfg->irq == IRQ_NONE) {
        return;
    }

    gpio_set_irq_polarity(cfg);
    DL_GPIO_clearInterruptStatus(cfg->port, cfg->pin_mask);
    DL_GPIO_enableInterrupt(cfg->port, cfg->pin_mask);
}

/*============================================================================
 *                        公开 API 实现
 *===========================================================================*/

HC_S32 HC_HAL_GPIO_Init(HC_VOID)
{
    HC_U8 i;
    HC_S32 ret;

    if (s_gpio_is_init == HC_TRUE) {
        return HC_HAL_ERR_ALREADY_INIT;
    }

    s_gpio_is_init = HC_FALSE;

    ret = gpio_build_irq_lut();
    if (ret != HC_HAL_OK) {
        return ret;
    }

    for (i = 0u; i < (HC_U8)HC_ARRAY_SIZE(s_pin_map); i++) {
        const PinCfg_t *cfg = &s_pin_map[i];

        gpio_cfg_direction(cfg);
        gpio_cfg_pull(cfg);
        gpio_cfg_irq(cfg);
    }

    s_gpio_is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_GPIO_SetPin(HC_HAL_GPIO_VPin_e vpin)
{
    HC_S32 ret = gpio_check_output_vpin(vpin);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    DL_GPIO_setPins(s_pin_map[vpin].port, s_pin_map[vpin].pin_mask);
    return HC_HAL_OK;
}

HC_S32 HC_HAL_GPIO_ResetPin(HC_HAL_GPIO_VPin_e vpin)
{
    HC_S32 ret = gpio_check_output_vpin(vpin);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    DL_GPIO_clearPins(s_pin_map[vpin].port, s_pin_map[vpin].pin_mask);
    return HC_HAL_OK;
}

HC_S32 HC_HAL_GPIO_TogglePin(HC_HAL_GPIO_VPin_e vpin)
{
    HC_S32 ret = gpio_check_output_vpin(vpin);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    DL_GPIO_togglePins(s_pin_map[vpin].port, s_pin_map[vpin].pin_mask);
    return HC_HAL_OK;
}

HC_S32 HC_HAL_GPIO_ReadPin(HC_HAL_GPIO_VPin_e vpin, HC_HAL_GPIO_PinState_e *p_state)
{
    HC_S32 ret;

    if (p_state == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    ret = gpio_check_vpin(vpin);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    *p_state = (DL_GPIO_readPins(s_pin_map[vpin].port, s_pin_map[vpin].pin_mask) != 0u) ? HC_PIN_SET : HC_PIN_RESET;
    return HC_HAL_OK;
}

/*============================================================================
 *                        中断转发与处理实现
 *===========================================================================*/

static HC_VOID gpio_irq_handler(GPIO_Regs *port, HC_U8 port_idx)
{
    HC_U32 status = DL_GPIO_getEnabledInterruptStatus(port, 0xFFFFFFFFu);

    DL_GPIO_clearInterruptStatus(port, status);

    while (status != 0u) {
        HC_U32 pin = status & (0u - status);
        HC_U8 bit_idx;
        HC_U8 lut_val;
        HC_HAL_GPIO_VPin_e vpin;

        status &= ~pin;

        if (gpio_pin_mask_to_bit_index(pin, &bit_idx) != HC_HAL_OK) {
            continue;
        }

        lut_val = s_gpio_irq_vpin_lut[port_idx][bit_idx];
        if (lut_val == HC_GPIO_VPIN_INVALID) {
            continue;
        }

        vpin = (HC_HAL_GPIO_VPin_e)lut_val;

        if (s_pin_map[vpin].irq == IRQ_BOTH) {
            gpio_set_irq_polarity(&s_pin_map[vpin]);
        }

        HC_HAL_GPIO_Callback(vpin);
    }
}

HC_VOID HC_HAL_GPIOA_IRQHandler(HC_VOID)
{
    gpio_irq_handler(GPIOA, 0u);
}

HC_VOID HC_HAL_GPIOB_IRQHandler(HC_VOID)
{
    gpio_irq_handler(GPIOB, 1u);
}

HC_WEAK HC_VOID HC_HAL_GPIO_Callback(HC_HAL_GPIO_VPin_e vpin)
{
    HC_UNUSED(vpin);
}
