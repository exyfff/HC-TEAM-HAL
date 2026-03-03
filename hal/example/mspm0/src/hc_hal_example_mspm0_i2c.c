/**
 * @file    hc_hal_example_mspm0_i2c.c
 * @brief   MSPM0 平台的 HC HAL I2C 参考实现。
 *
 * 本文件保留通用 HC HAL I2C 接口，并将其映射到 TI MSPM0 DriverLib。
 */

#include "ti_dl_lib.h"
#include "hc_hal_example_mspm0_board_cfg.h"
#include "hc_hal_i2c.h"
#include "hc_hal_systick.h"

typedef struct {
    I2C_Regs *base;
    GPIO_Regs *scl_port;
    HC_U32 scl_pin;
    HC_U32 scl_iomux;
    GPIO_Regs *sda_port;
    HC_U32 sda_pin;
    HC_U32 sda_iomux;
} I2cHwCfg_t;

typedef struct {
    HC_Bool_e is_init;
    HC_HAL_I2C_InitCfg_t cfg;
} I2cCtx_t;

#define HC_EXAMPLE_MSPM0_I2C_TRANSFER_IRQ_MASK                                                              \
    (DL_I2C_INTERRUPT_CONTROLLER_NACK | DL_I2C_INTERRUPT_CONTROLLER_STOP |                                 \
     DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST)

#if HC_HAL_I2C0_ENABLE
static const I2cHwCfg_t s_i2c0_hw = {(I2C_Regs *)HC_HAL_I2C0_BASE,
                                     HC_HAL_I2C0_SCL_PORT,
                                     HC_HAL_I2C0_SCL_PIN,
                                     HC_HAL_I2C0_SCL_IOMUX,
                                     HC_HAL_I2C0_SDA_PORT,
                                     HC_HAL_I2C0_SDA_PIN,
                                     HC_HAL_I2C0_SDA_IOMUX};
#endif

#if HC_HAL_I2C1_ENABLE
static const I2cHwCfg_t s_i2c1_hw = {(I2C_Regs *)HC_HAL_I2C1_BASE,
                                     HC_HAL_I2C1_SCL_PORT,
                                     HC_HAL_I2C1_SCL_PIN,
                                     HC_HAL_I2C1_SCL_IOMUX,
                                     HC_HAL_I2C1_SDA_PORT,
                                     HC_HAL_I2C1_SDA_PIN,
                                     HC_HAL_I2C1_SDA_IOMUX};
#endif

static I2cCtx_t s_i2c_ctx[HC_HAL_I2C_ID_MAX];

static HC_Bool_e i2c_is_valid_id(HC_HAL_I2C_Id_e id)
{
    return (id < HC_HAL_I2C_ID_MAX) ? HC_TRUE : HC_FALSE;
}

static const I2cHwCfg_t *i2c_get_hw_cfg(HC_HAL_I2C_Id_e id)
{
    switch (id) {
#if HC_HAL_I2C0_ENABLE
        case HC_HAL_I2C_ID_0:
            return &s_i2c0_hw;
#endif
#if HC_HAL_I2C1_ENABLE
        case HC_HAL_I2C_ID_1:
            return &s_i2c1_hw;
#endif
        default:
            return HC_NULL_PTR;
    }
}

static I2cCtx_t *i2c_get_ctx(HC_HAL_I2C_Id_e id)
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
    I2cCtx_t *p_ctx;

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

static HC_S32 i2c_calc_timer_period(HC_U32 bus_khz, HC_U8 *p_period)
{
    HC_U32 bus_hz;
    HC_U32 period_divider;

    if (p_period == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if ((HC_HAL_EXAMPLE_MSPM0_PERIPH_CLK_HZ == 0u) || (bus_khz == 0u)) {
        return HC_HAL_ERR_INVALID;
    }

    bus_hz = bus_khz * 1000u;
    period_divider = HC_HAL_EXAMPLE_MSPM0_PERIPH_CLK_HZ / (bus_hz * 10u);
    if (period_divider == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    period_divider -= 1u;
    if (period_divider > 0x7Fu) {
        return HC_HAL_ERR_INVALID;
    }

    *p_period = (HC_U8)period_divider;
    return HC_HAL_OK;
}

static HC_S32 i2c_check_timeout(HC_U32 start_ms, HC_U32 timeout_ms)
{
    HC_U32 now_ms;
    HC_S32 ret = HC_HAL_SYSTICK_GetTickMs(&now_ms);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    if ((now_ms - start_ms) >= timeout_ms) {
        return HC_HAL_ERR_TIMEOUT;
    }

    return HC_HAL_OK;
}

static HC_S32 i2c_check_transfer_error(const I2cHwCfg_t *p_hw)
{
    HC_U32 irq_status = DL_I2C_getRawInterruptStatus(p_hw->base, HC_EXAMPLE_MSPM0_I2C_TRANSFER_IRQ_MASK);

    if ((irq_status & DL_I2C_INTERRUPT_CONTROLLER_NACK) != 0u) {
        DL_I2C_clearInterruptStatus(p_hw->base, DL_I2C_INTERRUPT_CONTROLLER_NACK);
        return HC_HAL_ERR_NACK;
    }

    if ((irq_status & DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST) != 0u) {
        DL_I2C_clearInterruptStatus(p_hw->base, DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST);
        return HC_ERR_BUSY;
    }

    return HC_HAL_OK;
}

static HC_S32 i2c_wait_tx_ready(const I2cHwCfg_t *p_hw, HC_U32 start_ms, HC_U32 timeout_ms)
{
    HC_S32 ret;

    while (DL_I2C_isControllerTXFIFOFull(p_hw->base)) {
        ret = i2c_check_transfer_error(p_hw);
        if (ret != HC_HAL_OK) {
            return ret;
        }

        ret = i2c_check_timeout(start_ms, timeout_ms);
        if (ret != HC_HAL_OK) {
            return ret;
        }
    }

    return HC_HAL_OK;
}

static HC_S32 i2c_wait_rx_ready(const I2cHwCfg_t *p_hw, HC_U32 start_ms, HC_U32 timeout_ms)
{
    HC_S32 ret;

    while (DL_I2C_isControllerRXFIFOEmpty(p_hw->base)) {
        ret = i2c_check_transfer_error(p_hw);
        if (ret != HC_HAL_OK) {
            return ret;
        }

        ret = i2c_check_timeout(start_ms, timeout_ms);
        if (ret != HC_HAL_OK) {
            return ret;
        }
    }

    return HC_HAL_OK;
}

static HC_S32 i2c_wait_controller_idle_state(const I2cHwCfg_t *p_hw, HC_U32 start_ms, HC_U32 timeout_ms)
{
    HC_S32 ret;

    while ((DL_I2C_getControllerStatus(p_hw->base) & DL_I2C_CONTROLLER_STATUS_BUSY) != 0u) {
        ret = i2c_check_transfer_error(p_hw);
        if (ret != HC_HAL_OK) {
            return ret;
        }

        ret = i2c_check_timeout(start_ms, timeout_ms);
        if (ret != HC_HAL_OK) {
            return ret;
        }
    }

    return HC_HAL_OK;
}

static HC_S32 i2c_wait_stop(const I2cHwCfg_t *p_hw, HC_U32 start_ms, HC_U32 timeout_ms)
{
    HC_S32 ret;

    while (1) {
        HC_U32 irq_status = DL_I2C_getRawInterruptStatus(p_hw->base, HC_EXAMPLE_MSPM0_I2C_TRANSFER_IRQ_MASK);

        if ((irq_status & DL_I2C_INTERRUPT_CONTROLLER_NACK) != 0u) {
            DL_I2C_clearInterruptStatus(p_hw->base, DL_I2C_INTERRUPT_CONTROLLER_NACK);
            return HC_HAL_ERR_NACK;
        }

        if ((irq_status & DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST) != 0u) {
            DL_I2C_clearInterruptStatus(p_hw->base, DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST);
            return HC_ERR_BUSY;
        }

        if ((irq_status & DL_I2C_INTERRUPT_CONTROLLER_STOP) != 0u) {
            DL_I2C_clearInterruptStatus(p_hw->base, DL_I2C_INTERRUPT_CONTROLLER_STOP);
            return HC_HAL_OK;
        }

        ret = i2c_check_timeout(start_ms, timeout_ms);
        if (ret != HC_HAL_OK) {
            return ret;
        }
    }
}

static HC_S32 i2c_hw_init(const I2cHwCfg_t *p_hw, const HC_HAL_I2C_InitCfg_t *p_cfg)
{
    DL_I2C_ClockConfig clock_cfg;
    HC_U8 timer_period;
    HC_S32 ret;

    /*
     * 引脚复用默认由 SysConfig 或板级启动代码提前配置。
     * 本层只负责 I2C 控制器本身的 DriverLib 初始化。
     */
    ret = i2c_calc_timer_period(p_cfg->bus_khz, &timer_period);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    clock_cfg.clockSel = DL_I2C_CLOCK_BUSCLK;
    clock_cfg.divideRatio = DL_I2C_CLOCK_DIVIDE_1;

    DL_I2C_enablePower(p_hw->base);
    DL_I2C_reset(p_hw->base);
    DL_I2C_setClockConfig(p_hw->base, &clock_cfg);
    DL_I2C_setControllerAddressingMode(p_hw->base, DL_I2C_CONTROLLER_ADDRESSING_MODE_7_BIT);
    DL_I2C_setTimerPeriod(p_hw->base, timer_period);
    DL_I2C_flushControllerTXFIFO(p_hw->base);
    DL_I2C_flushControllerRXFIFO(p_hw->base);
    DL_I2C_clearInterruptStatus(p_hw->base, HC_EXAMPLE_MSPM0_I2C_TRANSFER_IRQ_MASK);
    DL_I2C_enableController(p_hw->base);
    return HC_HAL_OK;
}

static HC_S32 i2c_hw_deinit(const I2cHwCfg_t *p_hw)
{
    DL_I2C_clearInterruptStatus(p_hw->base, HC_EXAMPLE_MSPM0_I2C_TRANSFER_IRQ_MASK);
    DL_I2C_disableController(p_hw->base);
    DL_I2C_disablePower(p_hw->base);
    return HC_HAL_OK;
}

static HC_S32 i2c_hw_get_bus_idle(const I2cHwCfg_t *p_hw, HC_Bool_e *p_is_idle)
{
    if (p_is_idle == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    *p_is_idle = ((DL_I2C_getControllerStatus(p_hw->base) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) == 0u) ? HC_TRUE : HC_FALSE;
    return HC_HAL_OK;
}

static HC_S32 i2c_hw_write(const I2cHwCfg_t *p_hw,
                           HC_U8 dev_addr,
                           const HC_U8 *p_buf,
                           HC_U16 len,
                           HC_U32 timeout_ms)
{
    HC_U32 start_ms;
    HC_U16 i;
    HC_S32 ret = HC_HAL_SYSTICK_GetTickMs(&start_ms);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    DL_I2C_flushControllerTXFIFO(p_hw->base);
    DL_I2C_clearInterruptStatus(p_hw->base, HC_EXAMPLE_MSPM0_I2C_TRANSFER_IRQ_MASK);
    DL_I2C_startControllerTransfer(p_hw->base, dev_addr, DL_I2C_CONTROLLER_DIRECTION_TX, len);

    for (i = 0u; i < len; i++) {
        ret = i2c_wait_tx_ready(p_hw, start_ms, timeout_ms);
        if (ret != HC_HAL_OK) {
            return ret;
        }

        DL_I2C_transmitControllerData(p_hw->base, p_buf[i]);
    }

    return i2c_wait_stop(p_hw, start_ms, timeout_ms);
}

static HC_S32 i2c_hw_read(const I2cHwCfg_t *p_hw, HC_U8 dev_addr, HC_U8 *p_buf, HC_U16 len, HC_U32 timeout_ms)
{
    HC_U32 start_ms;
    HC_U16 i;
    HC_S32 ret = HC_HAL_SYSTICK_GetTickMs(&start_ms);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    DL_I2C_flushControllerRXFIFO(p_hw->base);
    DL_I2C_clearInterruptStatus(p_hw->base, HC_EXAMPLE_MSPM0_I2C_TRANSFER_IRQ_MASK);
    DL_I2C_startControllerTransfer(p_hw->base, dev_addr, DL_I2C_CONTROLLER_DIRECTION_RX, len);

    for (i = 0u; i < len; i++) {
        ret = i2c_wait_rx_ready(p_hw, start_ms, timeout_ms);
        if (ret != HC_HAL_OK) {
            return ret;
        }

        p_buf[i] = DL_I2C_receiveControllerData(p_hw->base);
    }

    return i2c_wait_stop(p_hw, start_ms, timeout_ms);
}

static HC_S32 i2c_hw_read_reg(const I2cHwCfg_t *p_hw,
                              HC_U8 dev_addr,
                              HC_U8 reg,
                              HC_U8 *p_val,
                              HC_U32 timeout_ms)
{
    HC_U32 start_ms;
    HC_S32 ret = HC_HAL_SYSTICK_GetTickMs(&start_ms);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    DL_I2C_flushControllerTXFIFO(p_hw->base);
    DL_I2C_flushControllerRXFIFO(p_hw->base);
    DL_I2C_clearInterruptStatus(p_hw->base, HC_EXAMPLE_MSPM0_I2C_TRANSFER_IRQ_MASK);

    DL_I2C_startControllerTransferAdvanced(p_hw->base,
                                           dev_addr,
                                           DL_I2C_CONTROLLER_DIRECTION_TX,
                                           1u,
                                           DL_I2C_CONTROLLER_START_ENABLE,
                                           DL_I2C_CONTROLLER_STOP_DISABLE,
                                           DL_I2C_CONTROLLER_ACK_ENABLE);

    ret = i2c_wait_tx_ready(p_hw, start_ms, timeout_ms);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    DL_I2C_transmitControllerData(p_hw->base, reg);

    ret = i2c_wait_controller_idle_state(p_hw, start_ms, timeout_ms);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    DL_I2C_clearInterruptStatus(p_hw->base, HC_EXAMPLE_MSPM0_I2C_TRANSFER_IRQ_MASK);
    DL_I2C_startControllerTransfer(p_hw->base, dev_addr, DL_I2C_CONTROLLER_DIRECTION_RX, 1u);

    ret = i2c_wait_rx_ready(p_hw, start_ms, timeout_ms);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    *p_val = DL_I2C_receiveControllerData(p_hw->base);
    return i2c_wait_stop(p_hw, start_ms, timeout_ms);
}

static HC_S32 i2c_wait_idle(const I2cCtx_t *p_ctx, const I2cHwCfg_t *p_hw)
{
    HC_U32 start_ms;
    HC_U32 now_ms;
    HC_S32 ret;

    ret = HC_HAL_SYSTICK_GetTickMs(&start_ms);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    while (1) {
        HC_Bool_e bus_idle = HC_FALSE;

        ret = i2c_hw_get_bus_idle(p_hw, &bus_idle);
        if (ret != HC_HAL_OK) {
            return ret;
        }

        if (bus_idle == HC_TRUE) {
            return HC_HAL_OK;
        }

        ret = HC_HAL_SYSTICK_GetTickMs(&now_ms);
        if (ret != HC_HAL_OK) {
            return ret;
        }

        if ((now_ms - start_ms) >= p_ctx->cfg.timeout_ms) {
            return HC_HAL_ERR_TIMEOUT;
        }
    }
}

HC_S32 HC_HAL_I2C_Init(HC_HAL_I2C_Handle_t *p_handle, HC_HAL_I2C_Id_e id, const HC_HAL_I2C_InitCfg_t *p_cfg)
{
    I2cCtx_t *p_ctx;
    const I2cHwCfg_t *p_hw;
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

    ret = i2c_hw_init(p_hw, p_cfg);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_handle->instance = (HC_U8)id;
    p_ctx->is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_I2C_Write(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, const HC_U8 *p_buf, HC_U16 len)
{
    I2cCtx_t *p_ctx;
    const I2cHwCfg_t *p_hw;
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
    p_hw = i2c_get_hw_cfg((HC_HAL_I2C_Id_e)p_handle->instance);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    ret = i2c_wait_idle(p_ctx, p_hw);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    return i2c_hw_write(p_hw, dev_addr, p_buf, len, p_ctx->cfg.timeout_ms);
}

HC_S32 HC_HAL_I2C_Read(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 *p_buf, HC_U16 len)
{
    I2cCtx_t *p_ctx;
    const I2cHwCfg_t *p_hw;
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
    p_hw = i2c_get_hw_cfg((HC_HAL_I2C_Id_e)p_handle->instance);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    ret = i2c_wait_idle(p_ctx, p_hw);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    return i2c_hw_read(p_hw, dev_addr, p_buf, len, p_ctx->cfg.timeout_ms);
}

HC_S32 HC_HAL_I2C_WriteReg(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 reg, HC_U8 val)
{
    HC_U8 tx_buf[2];

    tx_buf[0] = reg;
    tx_buf[1] = val;

    return HC_HAL_I2C_Write(p_handle, dev_addr, tx_buf, (HC_U16)HC_ARRAY_SIZE(tx_buf));
}

HC_S32 HC_HAL_I2C_ReadReg(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 reg, HC_U8 *p_val)
{
    I2cCtx_t *p_ctx;
    const I2cHwCfg_t *p_hw;
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
    p_hw = i2c_get_hw_cfg((HC_HAL_I2C_Id_e)p_handle->instance);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    ret = i2c_wait_idle(p_ctx, p_hw);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    return i2c_hw_read_reg(p_hw, dev_addr, reg, p_val, p_ctx->cfg.timeout_ms);
}

HC_S32 HC_HAL_I2C_DeInit(HC_HAL_I2C_Handle_t *p_handle)
{
    I2cCtx_t *p_ctx;
    const I2cHwCfg_t *p_hw;
    HC_S32 ret = i2c_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx = i2c_get_ctx((HC_HAL_I2C_Id_e)p_handle->instance);
    p_hw = i2c_get_hw_cfg((HC_HAL_I2C_Id_e)p_handle->instance);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    ret = i2c_hw_deinit(p_hw);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx->is_init = HC_FALSE;
    return HC_HAL_OK;
}
