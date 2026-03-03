/**
 * @file    hc_hal_example_stm32_uart.c
 * @brief   HC 团队 HAL UART 的 STM32 参考实现
 *
 * 本文件将主 HAL 的 UART 接口映射到标准 STM32 HAL。
 * 它使用 `UART_HandleTypeDef` 维护每个实例的运行期状态。
 */

#include "stm32xxxx_hal.h"
#include "hc_hal_example_stm32_board_cfg.h"
#include "hc_hal_uart.h"

typedef struct {
    USART_TypeDef *instance;
    GPIO_TypeDef *tx_port;
    HC_U16 tx_pin;
    HC_U32 tx_af;
    GPIO_TypeDef *rx_port;
    HC_U16 rx_pin;
    HC_U32 rx_af;
    IRQn_Type irqn;
} HcExampleStm32UartHwCfg_t;

typedef struct {
    HC_Bool_e is_init;
    HC_HAL_UART_InitCfg_t cfg;
    UART_HandleTypeDef hal_handle;
    HC_U8 rx_byte;
} HcExampleStm32UartCtx_t;

#if HC_HAL_EXAMPLE_STM32_UART0_ENABLE
static const HcExampleStm32UartHwCfg_t s_uart0_hw = {
    HC_HAL_EXAMPLE_STM32_UART0_INSTANCE,
    HC_HAL_EXAMPLE_STM32_UART0_TX_PORT,
    HC_HAL_EXAMPLE_STM32_UART0_TX_PIN,
    HC_HAL_EXAMPLE_STM32_UART0_TX_AF,
    HC_HAL_EXAMPLE_STM32_UART0_RX_PORT,
    HC_HAL_EXAMPLE_STM32_UART0_RX_PIN,
    HC_HAL_EXAMPLE_STM32_UART0_RX_AF,
    HC_HAL_EXAMPLE_STM32_UART0_IRQn,
};
#endif

#if HC_HAL_EXAMPLE_STM32_UART1_ENABLE
static const HcExampleStm32UartHwCfg_t s_uart1_hw = {
    HC_HAL_EXAMPLE_STM32_UART1_INSTANCE,
    HC_HAL_EXAMPLE_STM32_UART1_TX_PORT,
    HC_HAL_EXAMPLE_STM32_UART1_TX_PIN,
    HC_HAL_EXAMPLE_STM32_UART1_TX_AF,
    HC_HAL_EXAMPLE_STM32_UART1_RX_PORT,
    HC_HAL_EXAMPLE_STM32_UART1_RX_PIN,
    HC_HAL_EXAMPLE_STM32_UART1_RX_AF,
    HC_HAL_EXAMPLE_STM32_UART1_IRQn,
};
#endif

static HcExampleStm32UartCtx_t s_uart_ctx[HC_HAL_UART_ID_MAX];

static HC_U32 uart_get_hal_word_length(const HC_HAL_UART_InitCfg_t *p_cfg)
{
    if ((p_cfg->data_bits == 8u) && (p_cfg->parity != HC_HAL_UART_PARITY_NONE)) {
        return UART_WORDLENGTH_9B;
    }

    return UART_WORDLENGTH_8B;
}

static HC_S32 uart_map_hal_status(HAL_StatusTypeDef hal_status)
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

    return HC_HAL_ERR_NOT_READY;
}

static HC_S32 uart_hw_start_rx_irq(HcExampleStm32UartCtx_t *p_ctx)
{
    return uart_map_hal_status(HAL_UART_Receive_IT(&p_ctx->hal_handle, &p_ctx->rx_byte, 1u));
}

static HC_Bool_e uart_is_valid_id(HC_HAL_UART_Id_e id)
{
    return (id < HC_HAL_UART_ID_MAX) ? HC_TRUE : HC_FALSE;
}

static const HcExampleStm32UartHwCfg_t *uart_get_hw_cfg(HC_HAL_UART_Id_e id)
{
    switch (id) {
#if HC_HAL_EXAMPLE_STM32_UART0_ENABLE
        case HC_HAL_UART_ID_0:
            return &s_uart0_hw;
#endif
#if HC_HAL_EXAMPLE_STM32_UART1_ENABLE
        case HC_HAL_UART_ID_1:
            return &s_uart1_hw;
#endif
        default:
            return HC_NULL_PTR;
    }
}

static HcExampleStm32UartCtx_t *uart_get_ctx(HC_HAL_UART_Id_e id)
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
    HcExampleStm32UartCtx_t *p_ctx;

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

static HC_U32 uart_get_hal_parity(HC_HAL_UART_Parity_e parity)
{
    switch (parity) {
        case HC_HAL_UART_PARITY_ODD:
            return UART_PARITY_ODD;
        case HC_HAL_UART_PARITY_EVEN:
            return UART_PARITY_EVEN;
        case HC_HAL_UART_PARITY_NONE:
        default:
            return UART_PARITY_NONE;
    }
}

static HC_U32 uart_get_hal_stop_bits(HC_U8 stop_bits)
{
    return (stop_bits == 2u) ? UART_STOPBITS_2 : UART_STOPBITS_1;
}

static HC_VOID uart_enable_clock(HC_HAL_UART_Id_e id)
{
    switch (id) {
#if HC_HAL_EXAMPLE_STM32_UART0_ENABLE
        case HC_HAL_UART_ID_0:
            HC_HAL_EXAMPLE_STM32_UART0_CLK_ENABLE();
            break;
#endif
#if HC_HAL_EXAMPLE_STM32_UART1_ENABLE
        case HC_HAL_UART_ID_1:
            HC_HAL_EXAMPLE_STM32_UART1_CLK_ENABLE();
            break;
#endif
        default:
            break;
    }
}

static HC_S32 uart_hw_init(HC_HAL_UART_Id_e id, const HcExampleStm32UartHwCfg_t *p_hw, HcExampleStm32UartCtx_t *p_ctx)
{
    GPIO_InitTypeDef gpio_cfg;
    HC_S32 ret;

    HC_HAL_EXAMPLE_STM32_GPIOA_CLK_ENABLE();
    HC_HAL_EXAMPLE_STM32_GPIOB_CLK_ENABLE();
    HC_HAL_EXAMPLE_STM32_GPIOC_CLK_ENABLE();
    uart_enable_clock(id);

    gpio_cfg.Pin = p_hw->tx_pin;
    gpio_cfg.Mode = GPIO_MODE_AF_PP;
    gpio_cfg.Pull = GPIO_PULLUP;
    gpio_cfg.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_cfg.Alternate = p_hw->tx_af;
    HAL_GPIO_Init(p_hw->tx_port, &gpio_cfg);

    gpio_cfg.Pin = p_hw->rx_pin;
    gpio_cfg.Alternate = p_hw->rx_af;
    HAL_GPIO_Init(p_hw->rx_port, &gpio_cfg);

    p_ctx->hal_handle.Instance = p_hw->instance;
    p_ctx->hal_handle.Init.BaudRate = p_ctx->cfg.baud;
    p_ctx->hal_handle.Init.WordLength = uart_get_hal_word_length(&p_ctx->cfg);
    p_ctx->hal_handle.Init.StopBits = uart_get_hal_stop_bits(p_ctx->cfg.stop_bits);
    p_ctx->hal_handle.Init.Parity = uart_get_hal_parity(p_ctx->cfg.parity);
    p_ctx->hal_handle.Init.Mode = UART_MODE_TX_RX;
    p_ctx->hal_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    p_ctx->hal_handle.Init.OverSampling = UART_OVERSAMPLING_16;

    ret = uart_map_hal_status(HAL_UART_Init(&p_ctx->hal_handle));
    if (ret != HC_HAL_OK) {
        return ret;
    }

    if (p_ctx->cfg.rx_irq_enable == HC_ENABLE) {
        HAL_NVIC_SetPriority(p_hw->irqn, 5u, 0u);
        HAL_NVIC_EnableIRQ(p_hw->irqn);
    }

    return HC_HAL_OK;
}

static HC_S32 uart_hw_deinit(const HcExampleStm32UartHwCfg_t *p_hw, HcExampleStm32UartCtx_t *p_ctx)
{
    HC_S32 ret;

    if (p_ctx->cfg.rx_irq_enable == HC_ENABLE) {
        HAL_NVIC_DisableIRQ(p_hw->irqn);
    }

    HAL_GPIO_DeInit(p_hw->tx_port, p_hw->tx_pin);
    HAL_GPIO_DeInit(p_hw->rx_port, p_hw->rx_pin);

    ret = uart_map_hal_status(HAL_UART_DeInit(&p_ctx->hal_handle));
    if (ret != HC_HAL_OK) {
        return ret;
    }

    return HC_HAL_OK;
}

HC_S32 HC_HAL_UART_Init(HC_HAL_UART_Handle_t *p_handle, HC_HAL_UART_Id_e id, const HC_HAL_UART_InitCfg_t *p_cfg)
{
    HcExampleStm32UartCtx_t *p_ctx;
    const HcExampleStm32UartHwCfg_t *p_hw;
    HC_S32 ret;

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

    p_ctx = uart_get_ctx(id);
    p_hw = uart_get_hw_cfg(id);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    p_ctx->is_init = HC_FALSE;
    p_ctx->cfg = *p_cfg;
    if (p_ctx->cfg.rx_irq_enable == HC_DISABLE) {
        p_ctx->cfg.rx_callback = HC_NULL_FN;
    }

    ret = uart_hw_init(id, p_hw, p_ctx);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_handle->instance = (HC_U8)id;
    p_ctx->is_init = HC_TRUE;

    if (p_ctx->cfg.rx_irq_enable == HC_ENABLE) {
        ret = uart_hw_start_rx_irq(p_ctx);
        if (ret != HC_HAL_OK) {
            (HC_VOID) uart_hw_deinit(p_hw, p_ctx);
            p_ctx->is_init = HC_FALSE;
            return ret;
        }
    }

    return HC_HAL_OK;
}

HC_S32 HC_HAL_UART_Send(const HC_HAL_UART_Handle_t *p_handle, const HC_U8 *p_buf, HC_U16 len)
{
    HcExampleStm32UartCtx_t *p_ctx;
    HC_S32 ret = uart_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    if ((p_buf == HC_NULL_PTR) || (len == 0u)) {
        return (p_buf == HC_NULL_PTR) ? HC_HAL_ERR_NULL_PTR : HC_HAL_ERR_INVALID;
    }

    p_ctx = uart_get_ctx((HC_HAL_UART_Id_e)p_handle->instance);
    if (p_ctx == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    /*
     * 超时从波特率和数据长度自适应推算：
     * 每字节约 10 位（起始 + 8 数据 + 停止），再加 50ms 余量。
     * 最低保底 100ms，避免极端计算精度问题。
     */
    {
        HC_U32 timeout_ms = (((HC_U32)len * 10u * 1000u) / p_ctx->cfg.baud) + 50u;

        if (timeout_ms < 100u) {
            timeout_ms = 100u;
        }

        return uart_map_hal_status(HAL_UART_Transmit(&p_ctx->hal_handle, (HC_U8 *)p_buf, len, timeout_ms));
    }
}

HC_S32 HC_HAL_UART_SendByte(const HC_HAL_UART_Handle_t *p_handle, HC_U8 byte)
{
    HcExampleStm32UartCtx_t *p_ctx;
    HC_S32 ret = uart_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx = uart_get_ctx((HC_HAL_UART_Id_e)p_handle->instance);
    if (p_ctx == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    /* 单字节发送固定 100ms 超时，无需波特率推算 */
    return uart_map_hal_status(HAL_UART_Transmit(&p_ctx->hal_handle, &byte, 1u, 100u));
}

HC_S32 HC_HAL_UART_DeInit(HC_HAL_UART_Handle_t *p_handle)
{
    HcExampleStm32UartCtx_t *p_ctx;
    const HcExampleStm32UartHwCfg_t *p_hw;
    HC_S32 ret = uart_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx = uart_get_ctx((HC_HAL_UART_Id_e)p_handle->instance);
    p_hw = uart_get_hw_cfg((HC_HAL_UART_Id_e)p_handle->instance);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    ret = uart_hw_deinit(p_hw, p_ctx);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx->cfg.rx_callback = HC_NULL_FN;
    p_ctx->is_init = HC_FALSE;
    return HC_HAL_OK;
}

HC_VOID HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    HC_U8 i;

    for (i = 0u; i < (HC_U8)HC_HAL_UART_ID_MAX; i++) {
        HcExampleStm32UartCtx_t *p_ctx = &s_uart_ctx[i];

        if ((p_ctx->is_init == HC_TRUE) && (&p_ctx->hal_handle == huart)) {
            if (p_ctx->cfg.rx_callback != HC_NULL_FN) {
                p_ctx->cfg.rx_callback(p_ctx->rx_byte);
            }

            /*
             * 重启接收 IT：若失败（通常因 HAL 状态机错误或溢出），
             * 尝试清除溢出标志并复位 RxState 后重试一次。
             * 若仍失败则接收链中断，需上层通过 DeInit/Init 重新启动。
             */
            if (HAL_UART_Receive_IT(&p_ctx->hal_handle, &p_ctx->rx_byte, 1u) != HAL_OK) {
                __HAL_UART_CLEAR_OREFLAG(&p_ctx->hal_handle);
                p_ctx->hal_handle.RxState = HAL_UART_STATE_READY;
                (HC_VOID) HAL_UART_Receive_IT(&p_ctx->hal_handle, &p_ctx->rx_byte, 1u);
            }
        }
    }
}

HC_VOID HC_HAL_UART0_IRQHandler(HC_VOID)
{
#if HC_HAL_EXAMPLE_STM32_UART0_ENABLE
    HAL_UART_IRQHandler(&s_uart_ctx[HC_HAL_UART_ID_0].hal_handle);
#endif
}

HC_VOID HC_HAL_UART1_IRQHandler(HC_VOID)
{
#if HC_HAL_EXAMPLE_STM32_UART1_ENABLE
    HAL_UART_IRQHandler(&s_uart_ctx[HC_HAL_UART_ID_1].hal_handle);
#endif
}
