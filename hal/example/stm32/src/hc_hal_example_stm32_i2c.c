/**
 * @file    hc_hal_example_stm32_i2c.c
 * @brief   HC 团队 HAL I2C 的 STM32 参考实现
 *
 * 本文件将主 HAL 的阻塞式 I2C 接口映射到标准 STM32 HAL。
 * 它保留“未左移 7-bit 地址”的上层语义，并在底层调用前完成左移适配。
 */

#include "stm32xxxx_hal.h"
#include "hc_hal_example_stm32_board_cfg.h"
#include "hc_hal_i2c.h"

typedef struct {
    I2C_TypeDef *instance;
    GPIO_TypeDef *scl_port;
    HC_U16 scl_pin;
    HC_U32 scl_af;
    GPIO_TypeDef *sda_port;
    HC_U16 sda_pin;
    HC_U32 sda_af;
    HC_U32 timing;
    HC_U32 timing_400k;
} HcExampleStm32I2cHwCfg_t;

typedef struct {
    HC_Bool_e is_init;
    HC_HAL_I2C_InitCfg_t cfg;
    I2C_HandleTypeDef hal_handle;
} HcExampleStm32I2cCtx_t;

#if HC_HAL_EXAMPLE_STM32_I2C0_ENABLE
static const HcExampleStm32I2cHwCfg_t s_i2c0_hw = {
    HC_HAL_EXAMPLE_STM32_I2C0_INSTANCE,
    HC_HAL_EXAMPLE_STM32_I2C0_SCL_PORT,
    HC_HAL_EXAMPLE_STM32_I2C0_SCL_PIN,
    HC_HAL_EXAMPLE_STM32_I2C0_SCL_AF,
    HC_HAL_EXAMPLE_STM32_I2C0_SDA_PORT,
    HC_HAL_EXAMPLE_STM32_I2C0_SDA_PIN,
    HC_HAL_EXAMPLE_STM32_I2C0_SDA_AF,
    HC_HAL_EXAMPLE_STM32_I2C0_TIMING,
    HC_HAL_EXAMPLE_STM32_I2C0_TIMING_400K,
};
#endif

#if HC_HAL_EXAMPLE_STM32_I2C1_ENABLE
static const HcExampleStm32I2cHwCfg_t s_i2c1_hw = {
    HC_HAL_EXAMPLE_STM32_I2C1_INSTANCE,
    HC_HAL_EXAMPLE_STM32_I2C1_SCL_PORT,
    HC_HAL_EXAMPLE_STM32_I2C1_SCL_PIN,
    HC_HAL_EXAMPLE_STM32_I2C1_SCL_AF,
    HC_HAL_EXAMPLE_STM32_I2C1_SDA_PORT,
    HC_HAL_EXAMPLE_STM32_I2C1_SDA_PIN,
    HC_HAL_EXAMPLE_STM32_I2C1_SDA_AF,
    HC_HAL_EXAMPLE_STM32_I2C1_TIMING,
    HC_HAL_EXAMPLE_STM32_I2C1_TIMING_400K,
};
#endif

static HcExampleStm32I2cCtx_t s_i2c_ctx[HC_HAL_I2C_ID_MAX];

static HC_S32 i2c_map_hal_status(HAL_StatusTypeDef hal_status, I2C_HandleTypeDef *p_hal_handle)
{
    if (hal_status == HAL_OK) {
        return HC_HAL_OK;
    }

    if (hal_status == HAL_TIMEOUT) {
        return HC_HAL_ERR_TIMEOUT;
    }

    if (hal_status == HAL_BUSY) {
        return HC_ERR_BUSY;
    }

    if (p_hal_handle != HC_NULL_PTR) {
        HC_U32 error_code = HAL_I2C_GetError(p_hal_handle);

        if ((error_code & HAL_I2C_ERROR_AF) != 0u) {
            return HC_HAL_ERR_NACK;
        }
#ifdef HAL_I2C_ERROR_TIMEOUT
        if ((error_code & HAL_I2C_ERROR_TIMEOUT) != 0u) {
            return HC_HAL_ERR_TIMEOUT;
        }
#endif
    }

    return HC_HAL_ERR_NOT_READY;
}

static HC_Bool_e i2c_is_valid_id(HC_HAL_I2C_Id_e id)
{
    return (id < HC_HAL_I2C_ID_MAX) ? HC_TRUE : HC_FALSE;
}

static const HcExampleStm32I2cHwCfg_t *i2c_get_hw_cfg(HC_HAL_I2C_Id_e id)
{
    switch (id) {
#if HC_HAL_EXAMPLE_STM32_I2C0_ENABLE
        case HC_HAL_I2C_ID_0:
            return &s_i2c0_hw;
#endif
#if HC_HAL_EXAMPLE_STM32_I2C1_ENABLE
        case HC_HAL_I2C_ID_1:
            return &s_i2c1_hw;
#endif
        default:
            return HC_NULL_PTR;
    }
}

static HcExampleStm32I2cCtx_t *i2c_get_ctx(HC_HAL_I2C_Id_e id)
{
    if (i2c_is_valid_id(id) == HC_FALSE) {
        return HC_NULL_PTR;
    }

    return &s_i2c_ctx[(HC_U8)id];
}

static HC_S32 i2c_check_init_cfg(const HC_HAL_I2C_InitCfg_t *p_cfg)
{
    if (p_cfg == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if ((p_cfg->bus_khz == 0u) || (p_cfg->timeout_ms == 0u)) {
        return HC_HAL_ERR_INVALID;
    }

    return HC_HAL_OK;
}

static HC_S32 i2c_check_handle(const HC_HAL_I2C_Handle_t *p_handle)
{
    HcExampleStm32I2cCtx_t *p_ctx;

    if (p_handle == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (p_handle->instance >= (HC_U8)HC_HAL_I2C_ID_MAX) {
        return HC_HAL_ERR_INVALID;
    }

    if (i2c_get_hw_cfg((HC_HAL_I2C_Id_e)p_handle->instance) == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    p_ctx = &s_i2c_ctx[p_handle->instance];
    if (p_ctx->is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    return HC_HAL_OK;
}

static HC_S32 i2c_check_addr(HC_U8 dev_addr)
{
    if ((dev_addr < 0x08u) || (dev_addr > 0x77u)) {
        return HC_HAL_ERR_INVALID;
    }

    return HC_HAL_OK;
}

static HC_VOID i2c_enable_clock(HC_HAL_I2C_Id_e id)
{
    switch (id) {
#if HC_HAL_EXAMPLE_STM32_I2C0_ENABLE
        case HC_HAL_I2C_ID_0:
            HC_HAL_EXAMPLE_STM32_I2C0_CLK_ENABLE();
            break;
#endif
#if HC_HAL_EXAMPLE_STM32_I2C1_ENABLE
        case HC_HAL_I2C_ID_1:
            HC_HAL_EXAMPLE_STM32_I2C1_CLK_ENABLE();
            break;
#endif
        default:
            break;
    }
}

static HC_S32 i2c_hw_init(HC_HAL_I2C_Id_e id, const HcExampleStm32I2cHwCfg_t *p_hw, HcExampleStm32I2cCtx_t *p_ctx)
{
    GPIO_InitTypeDef gpio_cfg;
    HC_S32 ret;

    HC_HAL_EXAMPLE_STM32_GPIOA_CLK_ENABLE();
    HC_HAL_EXAMPLE_STM32_GPIOB_CLK_ENABLE();
    HC_HAL_EXAMPLE_STM32_GPIOC_CLK_ENABLE();
    i2c_enable_clock(id);

    gpio_cfg.Pin = p_hw->scl_pin;
    gpio_cfg.Mode = GPIO_MODE_AF_OD;
    gpio_cfg.Pull = GPIO_PULLUP;
    gpio_cfg.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_cfg.Alternate = p_hw->scl_af;
    HAL_GPIO_Init(p_hw->scl_port, &gpio_cfg);

    gpio_cfg.Pin = p_hw->sda_pin;
    gpio_cfg.Alternate = p_hw->sda_af;
    HAL_GPIO_Init(p_hw->sda_port, &gpio_cfg);

    p_ctx->hal_handle.Instance = p_hw->instance;
#if defined(I2C_TIMINGR_PRESC) || defined(I2C_TIMINGR_PRESC_Msk)
    /*
     * TIMINGR 路径（STM32F3/F7/G4/H7/L4 等新系列）：
     * Timing 寄存器值高度依赖内核时钟频率，无法简单从 bus_khz 线性计算。
     * 推荐使用 STM32CubeMX I2C Timing Configuration Tool 生成。
     *
     * 以下做基本的频率档位匹配：如果用户传入的 bus_khz 与板级预设档位匹配，
     * 则使用板级 Timing 值。否则回退到默认值并打印警告。
     */
    if (p_ctx->cfg.bus_khz <= 100u) {
        /* 标准模式 100kHz：使用板级预设 Timing */
        p_ctx->hal_handle.Init.Timing = p_hw->timing;
    } else if (p_ctx->cfg.bus_khz <= 400u) {
        /* 快速模式 400kHz：使用板级预设的 Fast Mode Timing */
        p_ctx->hal_handle.Init.Timing = p_hw->timing_400k;
    } else {
        /* 超出范围：回退到标准模式 Timing */
        p_ctx->hal_handle.Init.Timing = p_hw->timing;
    }
#else
    /* ClockSpeed 路径（STM32F1/F2/F4 等旧系列）：直接使用 bus_khz */
    p_ctx->hal_handle.Init.ClockSpeed = p_ctx->cfg.bus_khz * 1000u;
    p_ctx->hal_handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
#endif
    p_ctx->hal_handle.Init.OwnAddress1 = 0u;
    p_ctx->hal_handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    p_ctx->hal_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    p_ctx->hal_handle.Init.OwnAddress2 = 0u;
#if defined(I2C_TIMINGR_PRESC) || defined(I2C_TIMINGR_PRESC_Msk)
    p_ctx->hal_handle.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
#endif
    p_ctx->hal_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    p_ctx->hal_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    ret = i2c_map_hal_status(HAL_I2C_Init(&p_ctx->hal_handle), &p_ctx->hal_handle);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    return HC_HAL_OK;
}

static HC_S32 i2c_hw_deinit(const HcExampleStm32I2cHwCfg_t *p_hw, HcExampleStm32I2cCtx_t *p_ctx)
{
    HC_S32 ret;

    HAL_GPIO_DeInit(p_hw->scl_port, p_hw->scl_pin);
    HAL_GPIO_DeInit(p_hw->sda_port, p_hw->sda_pin);

    ret = i2c_map_hal_status(HAL_I2C_DeInit(&p_ctx->hal_handle), &p_ctx->hal_handle);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    return HC_HAL_OK;
}

HC_S32 HC_HAL_I2C_Init(HC_HAL_I2C_Handle_t *p_handle, HC_HAL_I2C_Id_e id, const HC_HAL_I2C_InitCfg_t *p_cfg)
{
    HcExampleStm32I2cCtx_t *p_ctx;
    const HcExampleStm32I2cHwCfg_t *p_hw;
    HC_S32 ret;

    if (p_handle == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (i2c_is_valid_id(id) == HC_FALSE) {
        return HC_HAL_ERR_INVALID;
    }

    ret = i2c_check_init_cfg(p_cfg);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx = i2c_get_ctx(id);
    p_hw = i2c_get_hw_cfg(id);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    p_ctx->is_init = HC_FALSE;
    p_ctx->cfg = *p_cfg;

    ret = i2c_hw_init(id, p_hw, p_ctx);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_handle->instance = (HC_U8)id;
    p_ctx->is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_I2C_Write(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, const HC_U8 *p_buf, HC_U16 len)
{
    HcExampleStm32I2cCtx_t *p_ctx;
    HC_S32 ret = i2c_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    ret = i2c_check_addr(dev_addr);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    if ((p_buf == HC_NULL_PTR) || (len == 0u)) {
        return (p_buf == HC_NULL_PTR) ? HC_HAL_ERR_NULL_PTR : HC_HAL_ERR_INVALID;
    }

    p_ctx = i2c_get_ctx((HC_HAL_I2C_Id_e)p_handle->instance);
    if (p_ctx == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    return i2c_map_hal_status(
        HAL_I2C_Master_Transmit(
            &p_ctx->hal_handle, (HC_U16)(dev_addr << 1), (HC_U8 *)p_buf, len, p_ctx->cfg.timeout_ms),
        &p_ctx->hal_handle);
}

HC_S32 HC_HAL_I2C_Read(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 *p_buf, HC_U16 len)
{
    HcExampleStm32I2cCtx_t *p_ctx;
    HC_S32 ret = i2c_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    ret = i2c_check_addr(dev_addr);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    if ((p_buf == HC_NULL_PTR) || (len == 0u)) {
        return (p_buf == HC_NULL_PTR) ? HC_HAL_ERR_NULL_PTR : HC_HAL_ERR_INVALID;
    }

    p_ctx = i2c_get_ctx((HC_HAL_I2C_Id_e)p_handle->instance);
    if (p_ctx == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    return i2c_map_hal_status(
        HAL_I2C_Master_Receive(&p_ctx->hal_handle, (HC_U16)(dev_addr << 1), p_buf, len, p_ctx->cfg.timeout_ms),
        &p_ctx->hal_handle);
}

HC_S32 HC_HAL_I2C_WriteReg(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 reg, HC_U8 val)
{
    HcExampleStm32I2cCtx_t *p_ctx;
    HC_S32 ret = i2c_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    ret = i2c_check_addr(dev_addr);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx = i2c_get_ctx((HC_HAL_I2C_Id_e)p_handle->instance);
    if (p_ctx == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    return i2c_map_hal_status(
        HAL_I2C_Mem_Write(
            &p_ctx->hal_handle, (HC_U16)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, &val, 1u, p_ctx->cfg.timeout_ms),
        &p_ctx->hal_handle);
}

HC_S32 HC_HAL_I2C_ReadReg(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 reg, HC_U8 *p_val)
{
    HcExampleStm32I2cCtx_t *p_ctx;
    HC_S32 ret;

    if (p_val == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    ret = i2c_check_handle(p_handle);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    ret = i2c_check_addr(dev_addr);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx = i2c_get_ctx((HC_HAL_I2C_Id_e)p_handle->instance);
    if (p_ctx == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    return i2c_map_hal_status(
        HAL_I2C_Mem_Read(
            &p_ctx->hal_handle, (HC_U16)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, p_val, 1u, p_ctx->cfg.timeout_ms),
        &p_ctx->hal_handle);
}

HC_S32 HC_HAL_I2C_DeInit(HC_HAL_I2C_Handle_t *p_handle)
{
    HcExampleStm32I2cCtx_t *p_ctx;
    const HcExampleStm32I2cHwCfg_t *p_hw;
    HC_S32 ret = i2c_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx = i2c_get_ctx((HC_HAL_I2C_Id_e)p_handle->instance);
    p_hw = i2c_get_hw_cfg((HC_HAL_I2C_Id_e)p_handle->instance);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    ret = i2c_hw_deinit(p_hw, p_ctx);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx->is_init = HC_FALSE;
    return HC_HAL_OK;
}
