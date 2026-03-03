/**
 * @file    hc_hal_example_mspm0_uart.c
 * @brief   MSPM0 平台的 HC HAL UART 参考实现。
 *
 * 本文件保留通用 HC HAL UART 接口，并将其映射到 TI MSPM0 DriverLib。
 */

#include "ti_dl_lib.h"
#include "hc_hal_example_mspm0_board_cfg.h"
#include "hc_hal_uart.h"
#include "hc_hal_systick.h"

typedef struct {
    UART_Regs *base;
    GPIO_Regs *tx_port;
    HC_U32 tx_pin;
    HC_U32 tx_iomux;
    GPIO_Regs *rx_port;
    HC_U32 rx_pin;
    HC_U32 rx_iomux;
    IRQn_Type irqn;
    HC_U32 irq_prio;
} UartHwCfg_t;

typedef struct {
    HC_Bool_e is_init;
    HC_HAL_UART_InitCfg_t cfg;
} UartCtx_t;

#define HC_EXAMPLE_MSPM0_UART_TX_TIMEOUT_MS 1000u
#define HC_EXAMPLE_MSPM0_UART_RX_IRQ_MASK                                                                              \
    (DL_UART_MAIN_INTERRUPT_RX | DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR | DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |      \
     DL_UART_MAIN_INTERRUPT_BREAK_ERROR | DL_UART_MAIN_INTERRUPT_PARITY_ERROR | DL_UART_MAIN_INTERRUPT_FRAMING_ERROR | \
     DL_UART_MAIN_INTERRUPT_NOISE_ERROR)

#if HC_HAL_UART0_ENABLE
static const UartHwCfg_t s_uart0_hw = {(UART_Regs *)HC_HAL_UART0_BASE,
                                       HC_HAL_UART0_TX_PORT,
                                       HC_HAL_UART0_TX_PIN,
                                       HC_HAL_UART0_TX_IOMUX,
                                       HC_HAL_UART0_RX_PORT,
                                       HC_HAL_UART0_RX_PIN,
                                       HC_HAL_UART0_RX_IOMUX,
                                       HC_HAL_UART0_IRQn,
                                       HC_HAL_UART0_IRQ_PRIO};
#endif

#if HC_HAL_UART1_ENABLE
static const UartHwCfg_t s_uart1_hw = {(UART_Regs *)HC_HAL_UART1_BASE,
                                       HC_HAL_UART1_TX_PORT,
                                       HC_HAL_UART1_TX_PIN,
                                       HC_HAL_UART1_TX_IOMUX,
                                       HC_HAL_UART1_RX_PORT,
                                       HC_HAL_UART1_RX_PIN,
                                       HC_HAL_UART1_RX_IOMUX,
                                       HC_HAL_UART1_IRQn,
                                       HC_HAL_UART1_IRQ_PRIO};
#endif

static UartCtx_t s_uart_ctx[HC_HAL_UART_ID_MAX];

static HC_Bool_e uart_is_valid_id(HC_HAL_UART_Id_e id)
{
    return (id < HC_HAL_UART_ID_MAX) ? HC_TRUE : HC_FALSE;
}

static const UartHwCfg_t *uart_get_hw_cfg(HC_HAL_UART_Id_e id)
{
    switch (id) {
#if HC_HAL_UART0_ENABLE
        case HC_HAL_UART_ID_0:
            return &s_uart0_hw;
#endif
#if HC_HAL_UART1_ENABLE
        case HC_HAL_UART_ID_1:
            return &s_uart1_hw;
#endif
        default:
            return HC_NULL_PTR;
    }
}

static UartCtx_t *uart_get_ctx(HC_HAL_UART_Id_e id)
{
    if (uart_is_valid_id(id) == HC_FALSE) {
        return HC_NULL_PTR;
    }

    return &s_uart_ctx[(HC_U8)id];
}

static HC_S32 uart_check_init_cfg(const HC_HAL_UART_InitCfg_t *p_cfg)
{
    if (p_cfg == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if ((p_cfg->baud == 0u) || (p_cfg->data_bits != 8u)) {
        return HC_HAL_ERR_INVALID;
    }

    if ((p_cfg->stop_bits != 1u) && (p_cfg->stop_bits != 2u)) {
        return HC_HAL_ERR_INVALID;
    }

    if (p_cfg->parity > HC_HAL_UART_PARITY_EVEN) {
        return HC_HAL_ERR_INVALID;
    }

    if ((p_cfg->rx_irq_enable == HC_ENABLE) && (p_cfg->rx_callback == HC_NULL_FN)) {
        return HC_HAL_ERR_INVALID;
    }

    return HC_HAL_OK;
}

static HC_S32 uart_check_handle(const HC_HAL_UART_Handle_t *p_handle)
{
    UartCtx_t *p_ctx;

    if (p_handle == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (p_handle->instance >= (HC_U8)HC_HAL_UART_ID_MAX) {
        return HC_HAL_ERR_INVALID;
    }

    if (uart_get_hw_cfg((HC_HAL_UART_Id_e)p_handle->instance) == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    p_ctx = &s_uart_ctx[p_handle->instance];
    if (p_ctx->is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    return HC_HAL_OK;
}

static DL_UART_PARITY uart_get_dl_parity(HC_HAL_UART_Parity_e parity)
{
    switch (parity) {
        case HC_HAL_UART_PARITY_ODD:
            return DL_UART_PARITY_ODD;

        case HC_HAL_UART_PARITY_EVEN:
            return DL_UART_PARITY_EVEN;

        case HC_HAL_UART_PARITY_NONE:
        default:
            return DL_UART_PARITY_NONE;
    }
}

static DL_UART_STOP_BITS uart_get_dl_stop_bits(HC_U8 stop_bits)
{
    return (stop_bits == 2u) ? DL_UART_STOP_BITS_TWO : DL_UART_STOP_BITS_ONE;
}

static HC_S32 uart_calc_baud_divisor(HC_U32 baud, HC_U32 *p_integer_div, HC_U32 *p_fractional_div)
{
    HC_U64 divisor_x64;

    if ((p_integer_div == HC_NULL_PTR) || (p_fractional_div == HC_NULL_PTR)) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if ((HC_HAL_EXAMPLE_MSPM0_PERIPH_CLK_HZ == 0u) || (baud == 0u)) {
        return HC_HAL_ERR_INVALID;
    }

    divisor_x64 = ((((HC_U64)HC_HAL_EXAMPLE_MSPM0_PERIPH_CLK_HZ) * 4u) + ((HC_U64)baud / 2u)) / (HC_U64)baud;
    if (divisor_x64 < 64u) {
        return HC_HAL_ERR_INVALID;
    }

    *p_integer_div = (HC_U32)(divisor_x64 / 64u);
    *p_fractional_div = (HC_U32)(divisor_x64 % 64u);
    return HC_HAL_OK;
}

static HC_S32 uart_hw_init(const UartHwCfg_t *p_hw, const HC_HAL_UART_InitCfg_t *p_cfg)
{
    DL_UART_Config uart_cfg;
    DL_UART_ClockConfig clock_cfg;
    HC_U32 integer_div;
    HC_U32 fractional_div;
    HC_S32 ret;

    /*
     * 引脚复用默认由 SysConfig 或板级启动代码提前配置。
     * 本层只负责 UART 外设本身的初始化。
     */
    ret = uart_calc_baud_divisor(p_cfg->baud, &integer_div, &fractional_div);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    uart_cfg.mode = DL_UART_MODE_NORMAL;
    uart_cfg.direction = DL_UART_DIRECTION_TX_RX;
    uart_cfg.flowControl = DL_UART_FLOW_CONTROL_NONE;
    uart_cfg.parity = uart_get_dl_parity(p_cfg->parity);
    uart_cfg.wordLength = DL_UART_WORD_LENGTH_8_BITS;
    uart_cfg.stopBits = uart_get_dl_stop_bits(p_cfg->stop_bits);

    clock_cfg.clockSel = DL_UART_CLOCK_BUSCLK;
    clock_cfg.divideRatio = DL_UART_CLOCK_DIVIDE_RATIO_1;

    DL_UART_Main_enablePower(p_hw->base);
    DL_UART_Main_reset(p_hw->base);
    DL_UART_Main_init(p_hw->base, &uart_cfg);
    DL_UART_Main_setClockConfig(p_hw->base, &clock_cfg);
    DL_UART_Main_setOversampling(p_hw->base, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(p_hw->base, integer_div, fractional_div);
    DL_UART_Main_clearInterruptStatus(p_hw->base, HC_EXAMPLE_MSPM0_UART_RX_IRQ_MASK);

    if (p_cfg->rx_irq_enable == HC_ENABLE) {
        DL_UART_Main_enableInterrupt(p_hw->base, HC_EXAMPLE_MSPM0_UART_RX_IRQ_MASK);
        NVIC_SetPriority(p_hw->irqn, p_hw->irq_prio);
        NVIC_EnableIRQ(p_hw->irqn);
    } else {
        DL_UART_Main_disableInterrupt(p_hw->base, HC_EXAMPLE_MSPM0_UART_RX_IRQ_MASK);
    }

    DL_UART_Main_enable(p_hw->base);
    return HC_HAL_OK;
}

static HC_S32 uart_hw_tx_byte(const UartHwCfg_t *p_hw, HC_U8 byte)
{
    HC_U32 start_ms = 0u;
    HC_Bool_e use_timeout = HC_FALSE;

    if (HC_HAL_SYSTICK_GetTickMs(&start_ms) == HC_HAL_OK) {
        use_timeout = HC_TRUE;
    }

    while (DL_UART_Main_isTXFIFOFull(p_hw->base)) {
        if (use_timeout == HC_TRUE) {
            HC_U32 now_ms;

            if (HC_HAL_SYSTICK_GetTickMs(&now_ms) != HC_HAL_OK) {
                use_timeout = HC_FALSE;
            } else if ((now_ms - start_ms) >= HC_EXAMPLE_MSPM0_UART_TX_TIMEOUT_MS) {
                return HC_HAL_ERR_TIMEOUT;
            }
        }
    }

    DL_UART_Main_transmitData(p_hw->base, byte);
    return HC_HAL_OK;
}

static HC_S32 uart_hw_deinit(const UartHwCfg_t *p_hw)
{
    NVIC_DisableIRQ(p_hw->irqn);
    DL_UART_Main_disableInterrupt(p_hw->base, HC_EXAMPLE_MSPM0_UART_RX_IRQ_MASK);
    DL_UART_Main_clearInterruptStatus(p_hw->base, HC_EXAMPLE_MSPM0_UART_RX_IRQ_MASK);
    DL_UART_Main_disable(p_hw->base);
    DL_UART_Main_reset(p_hw->base);
    DL_UART_Main_disablePower(p_hw->base);
    return HC_HAL_OK;
}

static HC_S32 uart_hw_rx_poll(const UartHwCfg_t *p_hw, HC_Bool_e *p_has_data, HC_U8 *p_data)
{
    DL_UART_IIDX pending_irq;

    if ((p_has_data == HC_NULL_PTR) || (p_data == HC_NULL_PTR)) {
        return HC_HAL_ERR_NULL_PTR;
    }

    *p_has_data = HC_FALSE;
    pending_irq = DL_UART_Main_getPendingInterrupt(p_hw->base);

    switch (pending_irq) {
        case DL_UART_MAIN_IIDX_RX:
            *p_data = DL_UART_Main_receiveData(p_hw->base);
            DL_UART_Main_clearInterruptStatus(p_hw->base, DL_UART_MAIN_INTERRUPT_RX);
            *p_has_data = HC_TRUE;
            break;

        case DL_UART_MAIN_IIDX_RX_TIMEOUT_ERROR:
            DL_UART_Main_clearInterruptStatus(p_hw->base, DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR);
            break;

        case DL_UART_MAIN_IIDX_OVERRUN_ERROR:
            DL_UART_Main_clearInterruptStatus(p_hw->base, DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR);
            break;

        case DL_UART_MAIN_IIDX_BREAK_ERROR:
            DL_UART_Main_clearInterruptStatus(p_hw->base, DL_UART_MAIN_INTERRUPT_BREAK_ERROR);
            break;

        case DL_UART_MAIN_IIDX_PARITY_ERROR:
            DL_UART_Main_clearInterruptStatus(p_hw->base, DL_UART_MAIN_INTERRUPT_PARITY_ERROR);
            break;

        case DL_UART_MAIN_IIDX_FRAMING_ERROR:
            DL_UART_Main_clearInterruptStatus(p_hw->base, DL_UART_MAIN_INTERRUPT_FRAMING_ERROR);
            break;

        case DL_UART_MAIN_IIDX_NOISE_ERROR:
            DL_UART_Main_clearInterruptStatus(p_hw->base, DL_UART_MAIN_INTERRUPT_NOISE_ERROR);
            break;

        default:
            break;
    }

    return HC_HAL_OK;
}

static HC_VOID uart_irq_handler(HC_HAL_UART_Id_e id)
{
    UartCtx_t *p_ctx = uart_get_ctx(id);
    const UartHwCfg_t *p_hw = uart_get_hw_cfg(id);
    HC_Bool_e has_rx_data = HC_FALSE;
    HC_U8 rx_data = 0u;

    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return;
    }

    if ((p_ctx->is_init == HC_FALSE) || (p_ctx->cfg.rx_irq_enable != HC_ENABLE)) {
        return;
    }

    if (uart_hw_rx_poll(p_hw, &has_rx_data, &rx_data) != HC_HAL_OK) {
        return;
    }

    if ((has_rx_data == HC_TRUE) && (p_ctx->cfg.rx_callback != HC_NULL_FN)) {
        p_ctx->cfg.rx_callback(rx_data);
    }
}

HC_S32 HC_HAL_UART_Init(HC_HAL_UART_Handle_t *p_handle, HC_HAL_UART_Id_e id, const HC_HAL_UART_InitCfg_t *p_cfg)
{
    HC_S32 ret;
    UartCtx_t *p_ctx;
    const UartHwCfg_t *p_hw;

    if (p_handle == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (uart_is_valid_id(id) == HC_FALSE) {
        return HC_HAL_ERR_INVALID;
    }

    ret = uart_check_init_cfg(p_cfg);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_hw = uart_get_hw_cfg(id);
    p_ctx = uart_get_ctx(id);
    if ((p_hw == HC_NULL_PTR) || (p_ctx == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    p_ctx->is_init = HC_FALSE;
    p_ctx->cfg = *p_cfg;

    ret = uart_hw_init(p_hw, &p_ctx->cfg);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_handle->instance = (HC_U8)id;
    p_ctx->is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_UART_Send(const HC_HAL_UART_Handle_t *p_handle, const HC_U8 *p_buf, HC_U16 len)
{
    const UartHwCfg_t *p_hw;
    HC_U16 i;
    HC_S32 ret;

    ret = uart_check_handle(p_handle);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    if ((p_buf == HC_NULL_PTR) || (len == 0u)) {
        return (p_buf == HC_NULL_PTR) ? HC_HAL_ERR_NULL_PTR : HC_HAL_ERR_INVALID;
    }

    p_hw = uart_get_hw_cfg((HC_HAL_UART_Id_e)p_handle->instance);
    if (p_hw == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    for (i = 0u; i < len; i++) {
        ret = uart_hw_tx_byte(p_hw, p_buf[i]);
        if (ret != HC_HAL_OK) {
            return ret;
        }
    }

    return HC_HAL_OK;
}

HC_S32 HC_HAL_UART_SendByte(const HC_HAL_UART_Handle_t *p_handle, HC_U8 byte)
{
    const UartHwCfg_t *p_hw;
    HC_S32 ret = uart_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_hw = uart_get_hw_cfg((HC_HAL_UART_Id_e)p_handle->instance);
    if (p_hw == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    return uart_hw_tx_byte(p_hw, byte);
}

HC_S32 HC_HAL_UART_DeInit(HC_HAL_UART_Handle_t *p_handle)
{
    UartCtx_t *p_ctx;
    const UartHwCfg_t *p_hw;
    HC_S32 ret = uart_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx = uart_get_ctx((HC_HAL_UART_Id_e)p_handle->instance);
    p_hw = uart_get_hw_cfg((HC_HAL_UART_Id_e)p_handle->instance);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    ret = uart_hw_deinit(p_hw);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx->cfg.rx_callback = HC_NULL_FN;
    p_ctx->is_init = HC_FALSE;
    return HC_HAL_OK;
}

HC_VOID HC_HAL_UART0_IRQHandler(HC_VOID)
{
    uart_irq_handler(HC_HAL_UART_ID_0);
}

HC_VOID HC_HAL_UART1_IRQHandler(HC_VOID)
{
    uart_irq_handler(HC_HAL_UART_ID_1);
}
