/**
 * @file    hc_hal_uart.c
 * @brief   HC 团队 HAL UART 模板实现
 */

/*============================================================================
 *                        包含头文件
 *===========================================================================*/
/* 本文件是主 HAL 模板层，不直接依赖任何厂商 SDK。 */
/* 移植到具体平台时，请在对应的 *_hw_* 私有函数中接入目标平台驱动调用。 */
#include "hc_hal_board_cfg.h"
#include "hc_hal_uart.h"
#include "hc_hal_systick.h"

/*============================================================================
 *                        内部私有定义
 *===========================================================================*/

/**
 * @brief UART 硬件资源映射结构体
 */
typedef struct {
    HC_VOID *base;    /* 外设基址或平台私有控制块指针 */
    HC_VOID *tx_port; /* TX 端口句柄/端口标识 */
    HC_U32 tx_pin;    /* TX 引脚编号或位掩码 */
    HC_U32 tx_iomux;  /* TX 复用配置或平台私有选择值 */
    HC_VOID *rx_port; /* RX 端口句柄/端口标识 */
    HC_U32 rx_pin;    /* RX 引脚编号或位掩码 */
    HC_U32 rx_iomux;  /* RX 复用配置或平台私有选择值 */
} UartHwCfg_t;

/**
 * @brief UART 运行期上下文状态结构体
 */
typedef struct {
    HC_Bool_e is_init;         /* 实例初始化状态标记 */
    HC_HAL_UART_InitCfg_t cfg; /* 当前绑定的逻辑配置备份 */
} UartCtx_t;

/*============================================================================
 *                        私有辅助函数声明
 *===========================================================================*/
static HC_Bool_e uart_is_valid_id(HC_HAL_UART_Id_e id);
static const UartHwCfg_t *uart_get_hw_cfg(HC_HAL_UART_Id_e id);
static UartCtx_t *uart_get_ctx(HC_HAL_UART_Id_e id);
static HC_S32 uart_check_init_cfg(const HC_HAL_UART_InitCfg_t *p_cfg);
static HC_S32 uart_check_handle(const HC_HAL_UART_Handle_t *p_handle);
static HC_S32 uart_check_tx_buffer(const HC_HAL_UART_Handle_t *p_handle, const HC_U8 *p_buf, HC_U16 len);
static HC_S32 uart_hw_init(HC_HAL_UART_Id_e id, const UartHwCfg_t *p_hw, const HC_HAL_UART_InitCfg_t *p_cfg);
static HC_S32 uart_hw_tx_byte(HC_HAL_UART_Id_e id, const UartHwCfg_t *p_hw, HC_U8 byte);
static HC_S32 uart_hw_deinit(HC_HAL_UART_Id_e id, const UartHwCfg_t *p_hw);
static HC_S32 uart_hw_rx_poll(const UartHwCfg_t *p_hw, HC_Bool_e *p_has_data, HC_U8 *p_data);
static HC_VOID uart_irq_handler(HC_HAL_UART_Id_e id);

/*============================================================================
 *                        私有全局变量/配置表
 *===========================================================================*/

#if HC_HAL_UART0_ENABLE
/** @brief UART 实例 0 硬件配置表 */
static const UartHwCfg_t s_uart0_hw = {(HC_VOID *)HC_HAL_UART0_BASE,
                                       HC_HAL_UART0_TX_PORT,
                                       HC_HAL_UART0_TX_PIN,
                                       HC_HAL_UART0_TX_IOMUX,
                                       HC_HAL_UART0_RX_PORT,
                                       HC_HAL_UART0_RX_PIN,
                                       HC_HAL_UART0_RX_IOMUX};
#endif

#if HC_HAL_UART1_ENABLE
/** @brief UART 实例 1 硬件配置表 */
static const UartHwCfg_t s_uart1_hw = {(HC_VOID *)HC_HAL_UART1_BASE,
                                       HC_HAL_UART1_TX_PORT,
                                       HC_HAL_UART1_TX_PIN,
                                       HC_HAL_UART1_TX_IOMUX,
                                       HC_HAL_UART1_RX_PORT,
                                       HC_HAL_UART1_RX_PIN,
                                       HC_HAL_UART1_RX_IOMUX};
#endif

/** @brief 模块内部上下文数组 */
static UartCtx_t s_uart_ctx[HC_HAL_UART_ID_MAX];

/*============================================================================
 *                        私有辅助函数实现
 *===========================================================================*/

/**
 * @brief 判断 UART ID 是否在合法枚举范围内
 * @param id 目标实例 ID
 * @return HC_TRUE 为有效，否则为 HC_FALSE
 */
static HC_Bool_e uart_is_valid_id(HC_HAL_UART_Id_e id)
{
    return (id < HC_HAL_UART_ID_MAX) ? HC_TRUE : HC_FALSE;
}

/**
 * @brief 根据实例 ID 获取硬件配置描述符指针
 * @param id 目标实例 ID
 * @return 成功返回描述符指针，不使能或 ID 错误返回 HC_NULL_PTR
 */
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

/**
 * @brief 获取当前实例的上下文内存指针
 * @param id 目标实例 ID
 * @return 上下文指针，ID 非法返回 HC_NULL_PTR
 */
static UartCtx_t *uart_get_ctx(HC_HAL_UART_Id_e id)
{
    if (uart_is_valid_id(id) == HC_FALSE) {
        return HC_NULL_PTR;
    }

    return &s_uart_ctx[(HC_U8)id];
}

/**
 * @brief 深度校验初始化配置参数的合法性
 * @param p_cfg 待检查的配置指针
 * @return HC_HAL_OK 为合法，否则返回错误码
 */
static HC_S32 uart_check_init_cfg(const HC_HAL_UART_InitCfg_t *p_cfg)
{
    if (p_cfg == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (p_cfg->baud == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    /* 约束：当前示例模板限制仅支持 8 位数据宽度 */
    if (p_cfg->data_bits != 8u) {
        return HC_HAL_ERR_INVALID;
    }

    if ((p_cfg->stop_bits != 1u) && (p_cfg->stop_bits != 2u)) {
        return HC_HAL_ERR_INVALID;
    }

    if (p_cfg->parity > HC_HAL_UART_PARITY_EVEN) {
        return HC_HAL_ERR_INVALID;
    }

    if ((p_cfg->rx_irq_enable != HC_DISABLE) && (p_cfg->rx_irq_enable != HC_ENABLE)) {
        return HC_HAL_ERR_INVALID;
    }

    if ((p_cfg->rx_irq_enable == HC_ENABLE) && (p_cfg->rx_callback == HC_NULL_FN)) {
        return HC_HAL_ERR_INVALID;
    }

    return HC_HAL_OK;
}

/**
 * @brief 校验句柄的有效性及其绑定的实例状态
 * @param p_handle 句柄指针
 * @return HC_HAL_OK 为校验通过
 */
static HC_S32 uart_check_handle(const HC_HAL_UART_Handle_t *p_handle)
{
    UartCtx_t *p_ctx;

    if (p_handle == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (p_handle->instance >= (HC_U8)HC_HAL_UART_ID_MAX) {
        return HC_HAL_ERR_INVALID;
    }

    /* 检查硬件使能标记：防止通过句柄访问未打开的实例 */
    if (uart_get_hw_cfg((HC_HAL_UART_Id_e)p_handle->instance) == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    p_ctx = &s_uart_ctx[p_handle->instance];
    if (p_ctx->is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    return HC_HAL_OK;
}

/**
 * @brief 针对发送操作的数据缓冲区校验
 * @param p_handle 句柄指针
 * @param p_buf 发送缓冲区
 * @param len 发送长度
 */
static HC_S32 uart_check_tx_buffer(const HC_HAL_UART_Handle_t *p_handle, const HC_U8 *p_buf, HC_U16 len)
{
    HC_S32 ret = uart_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    if (p_buf == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (len == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    return HC_HAL_OK;
}

/**
 * @brief 底层 UART 硬件初始化
 *
 * 调用前须确保实例号、句柄和配置都已完成上层校验。
 *
 * @param id    目标实例 ID
 * @param p_hw  硬件配置描述符
 * @param p_cfg 初始化配置
 * @return HC_S32 成功返回 HC_HAL_OK，未适配返回 HC_ERR_NOT_READY
 */
static HC_S32 uart_hw_init(HC_HAL_UART_Id_e id, const UartHwCfg_t *p_hw, const HC_HAL_UART_InitCfg_t *p_cfg)
{
    HC_UNUSED(id);
    HC_UNUSED(p_hw);
    HC_UNUSED(p_cfg);

    /*
     * 待适配: 在此处填充 UART 硬件初始化时序。
     * 替换流程:
     * 1. 使能目标外设(UART/GPIO)的系统级总线时钟。
     * 2. 配置引脚复用选择至 UART TX/RX 功能。
     * 3. 设置分频器或波特率寄存器。
     * 4. 如果 rx_irq_enable 为 HC_ENABLE，则清除挂起标志并使能 RX 中断。
     */
    return HC_ERR_NOT_READY;
}

/**
 * @brief 底层单字节硬件发送
 *
 * 调用前须确保句柄与硬件配置已经过校验。
 *
 * @param id    目标实例 ID
 * @param p_hw  硬件配置描述符
 * @param byte  待写入发送寄存器的数据
 * @return HC_S32 成功返回 HC_HAL_OK，未适配返回 HC_ERR_NOT_READY
 */
static HC_S32 uart_hw_tx_byte(HC_HAL_UART_Id_e id, const UartHwCfg_t *p_hw, HC_U8 byte)
{
    HC_UNUSED(id);
    HC_UNUSED(byte);
    HC_UNUSED(p_hw);

    /*
     * 待适配 [重要]: 在此处插入具体芯片的阻塞发送实现逻辑。
     * 必须包含超时保护，否则硬件异常时将导致死锁。
     * 替换流程:
     * 1. 通过 HC_HAL_SYSTICK_GetTickMs() 获取起始时间。
     * 2. 循环等待 TX FIFO 非满 / 发送移位寄存器就绪。
     * 3. 每轮循环计算已用时间，超时则返回 HC_HAL_ERR_TIMEOUT。
     * 4. 就绪后将 byte 写入外设的数据发送寄存器。
     * 5. 当前仅为裸机阻塞模板；若迁移到 RTOS，请改为队列 + 中断/DMA。
     */
    return HC_ERR_NOT_READY;
}

/**
 * @brief 底层 UART 反初始化入口
 *
 * 该函数是当前通用 HAL 模板中承接平台库/底层 API 反初始化调用的集中封装点。
 * 若迁移到具体芯片，请仅在此处接入目标平台的 UART 关闭流程。
 *
 * @param id   目标实例 ID
 * @param p_hw 硬件配置描述符
 * @return HC_S32 成功返回 HC_HAL_OK，未适配返回 HC_ERR_NOT_READY
 */
static HC_S32 uart_hw_deinit(HC_HAL_UART_Id_e id, const UartHwCfg_t *p_hw)
{
    HC_UNUSED(id);
    HC_UNUSED(p_hw);

    /*
     * 待适配: 在此处填充 UART 硬件反初始化流程。
     * 建议替换流程:
     * 1. 关闭 RX/TX 中断源。
     * 2. 停止 UART 外设并清理挂起标志。
     * 3. 关闭 UART 外设时钟以降低功耗。
     * 4. 将 TX/RX 引脚复用恢复为默认 GPIO 状态。
     */
    return HC_ERR_NOT_READY;
}

/**
 * @brief 底层 RX 中断数据提取
 *
 * 在 ISR 上下文中调用，负责判断中断来源、清除标志并读取接收数据。
 *
 * @param p_hw       硬件配置描述符
 * @param p_has_data 输出：本次中断是否包含有效 RX 数据
 * @param p_data     输出：接收到的单字节数据
 * @return HC_S32    成功返回 HC_HAL_OK，未适配返回 HC_ERR_NOT_READY
 */
static HC_S32 uart_hw_rx_poll(const UartHwCfg_t *p_hw, HC_Bool_e *p_has_data, HC_U8 *p_data)
{
    HC_UNUSED(p_hw);
    HC_UNUSED(p_has_data);
    HC_UNUSED(p_data);

    /*
     * 待适配: 在此处实现接收中断数据提取。
     * 替换流程:
     * 1. 确认本次中断是否来自接收 (RX) 事件（而非发送空中断等）。
     * 2. 向控制器确认标志并手动清除。
     * 3. 读取外设 RX 寄存器数据至 *p_data。
     * 4. 读取成功后将 *p_has_data 置为 HC_TRUE，否则置为 HC_FALSE。
     * 5. 若后续迁移到 RTOS，请避免在 ISR 中执行复杂业务逻辑，建议只做搬运并投递事件。
     */
    return HC_ERR_NOT_READY;
}

/*============================================================================
 *                        公开 API 函数实现
 *===========================================================================*/

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
    if (p_hw == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    p_ctx = uart_get_ctx(id);
    if (p_ctx == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    /* 状态同步：回退至未初始化状态，防止后续配置崩溃导致的逻辑脏态 */
    p_ctx->is_init = HC_FALSE;
    p_ctx->cfg = *p_cfg;
    if (p_ctx->cfg.rx_irq_enable == HC_DISABLE) {
        p_ctx->cfg.rx_callback = HC_NULL_FN;
    }

    ret = uart_hw_init(id, p_hw, &p_ctx->cfg);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    /* 仅在初始化成功后再向外导出实例号，避免失败时留下半初始化句柄。 */
    p_handle->instance = (HC_U8)id;
    p_ctx->is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_UART_Send(const HC_HAL_UART_Handle_t *p_handle, const HC_U8 *p_buf, HC_U16 len)
{
    HC_S32 ret = uart_check_tx_buffer(p_handle, p_buf, len);
    const UartHwCfg_t *p_hw;
    HC_U16 i;

    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_hw = uart_get_hw_cfg((HC_HAL_UART_Id_e)p_handle->instance);
    if (p_hw == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    /*
     * 发送流策略：入口校验一次，循环内直接操作硬件。
     * 当前仅供裸机参考；若迁移到 RTOS，请改为队列、中断或 DMA 驱动，而不是在任务中持续自旋。
     */
    for (i = 0u; i < len; i++) {
        ret = uart_hw_tx_byte((HC_HAL_UART_Id_e)p_handle->instance, p_hw, p_buf[i]);
        if (ret != HC_HAL_OK) {
            return ret;
        }
    }

    return HC_HAL_OK;
}

HC_S32 HC_HAL_UART_SendByte(const HC_HAL_UART_Handle_t *p_handle, HC_U8 byte)
{
    HC_S32 ret = uart_check_handle(p_handle);
    const UartHwCfg_t *p_hw;

    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_hw = uart_get_hw_cfg((HC_HAL_UART_Id_e)p_handle->instance);
    if (p_hw == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    return uart_hw_tx_byte((HC_HAL_UART_Id_e)p_handle->instance, p_hw, byte);
}

HC_S32 HC_HAL_UART_DeInit(HC_HAL_UART_Handle_t *p_handle)
{
    HC_S32 ret;
    UartCtx_t *p_ctx;
    const UartHwCfg_t *p_hw;

    if (p_handle == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    ret = uart_check_handle(p_handle);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx = uart_get_ctx((HC_HAL_UART_Id_e)p_handle->instance);
    p_hw = uart_get_hw_cfg((HC_HAL_UART_Id_e)p_handle->instance);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    /* 通用 HAL 层只负责组织反初始化流程，平台相关寄存器/SDK 调用统一下沉到底层 helper。 */
    ret = uart_hw_deinit((HC_HAL_UART_Id_e)p_handle->instance, p_hw);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx->cfg.rx_callback = HC_NULL_FN;
    p_ctx->is_init = HC_FALSE;
    return HC_HAL_OK;
}

/*============================================================================
 *                        中断转发与处理实现
 *===========================================================================*/

/**
 * @brief 通用 UART 逻辑中断处理器
 * @param id 触发中断的实例 ID
 */
static HC_VOID uart_irq_handler(HC_HAL_UART_Id_e id)
{
    UartCtx_t *p_ctx = uart_get_ctx(id);
    const UartHwCfg_t *p_hw = uart_get_hw_cfg(id);
    HC_Bool_e has_rx_data = HC_FALSE;
    HC_U8 rx_data = 0u;

    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return;
    }

    if (p_ctx->is_init == HC_FALSE) {
        return;
    }

    if (p_ctx->cfg.rx_irq_enable != HC_ENABLE) {
        /* 该实例未启用 RX 中断；若仍进入此路径，通常说明底层中断源配置与逻辑配置不一致。 */
        return;
    }

    /* 调用底层硬件抽象接口完成中断源判定、标志清除与数据提取 */
    if (uart_hw_rx_poll(p_hw, &has_rx_data, &rx_data) != HC_HAL_OK) {
        return;
    }

    /* 将捕获到的数据通过该实例独占的逻辑回调指针透传至用户层 */
    if ((has_rx_data == HC_TRUE) && (p_ctx->cfg.rx_callback != HC_NULL_FN)) {
        p_ctx->cfg.rx_callback(rx_data);
    }
}

HC_VOID HC_HAL_UART0_IRQHandler(HC_VOID)
{
    /* 即使实例未启用也保留该入口，避免未知启动文件环境下缺少中断符号。 */
    uart_irq_handler(HC_HAL_UART_ID_0);
}

HC_VOID HC_HAL_UART1_IRQHandler(HC_VOID)
{
    /* 即使实例未启用也保留该入口，避免未知启动文件环境下缺少中断符号。 */
    uart_irq_handler(HC_HAL_UART_ID_1);
}
