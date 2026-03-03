/**
 * @file    hc_hal_uart.h
 * @brief   HC 团队 HAL UART 对外接口
 */

#ifndef HC_HAL_UART_H
#define HC_HAL_UART_H

#include "hc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *                        宏定义与枚举
 *===========================================================================*/

/**
 * @brief UART 中断向量名映射
 *
 * 以下宏必须在板级配置 (board_cfg.h) 中根据目标平台定义，例如：
 * - MSPM0: #define HC_HAL_UART0_IRQHandler UART0_INT_IRQHandler
 * - STM32: #define HC_HAL_UART0_IRQHandler USART1_IRQHandler
 */
#ifndef HC_HAL_UART0_IRQHandler
#error "HC_HAL_UART0_IRQHandler must be defined in board_cfg.h"
#endif

#ifndef HC_HAL_UART1_IRQHandler
#error "HC_HAL_UART1_IRQHandler must be defined in board_cfg.h"
#endif

/**
 * @brief UART 硬件实例 ID 定义
 */
typedef enum {
    HC_HAL_UART_ID_0 = 0, /* UART 实例 0 (通常映射为 UART0) */
    HC_HAL_UART_ID_1 = 1, /* UART 实例 1 (通常映射为 UART1) */
    HC_HAL_UART_ID_MAX    /* 实例 ID 最大值标记 */
} HC_HAL_UART_Id_e;

/**
 * @brief UART 校验位配置枚举
 */
typedef enum {
    HC_HAL_UART_PARITY_NONE = 0, /* 无校验 */
    HC_HAL_UART_PARITY_ODD = 1,  /* 奇校验 */
    HC_HAL_UART_PARITY_EVEN = 2  /* 偶校验 */
} HC_HAL_UART_Parity_e;

/**
 * @brief UART 接收回调函数指针类型
 * @param data 接收到的 1 字节数据
 */
typedef HC_VOID (*HC_HAL_UART_RxCallback_t)(HC_U8 data);

/*============================================================================
 *                        数据结构定义
 *===========================================================================*/

/**
 * @brief UART 控制句柄结构体
 * 
 * 上层应用需为每个 UART 实例分配一个句柄变量，用于后续的读写操作。
 */
typedef struct {
    HC_U8 instance; /* 绑定的硬件实例索引 (HC_HAL_UART_Id_e) */
} HC_HAL_UART_Handle_t;

/**
 * @brief UART 初始化配置参数结构体
 */
typedef struct {
    HC_U32 baud;                          /* 波特率 (如 115200) */
    HC_U8 data_bits;                      /* 数据位 (通常为 8) */
    HC_U8 stop_bits;                      /* 停止位 (1 或 2) */
    HC_HAL_UART_Parity_e parity;          /* 校验位配置 */
    HC_Enable_e rx_irq_enable;            /* 接收中断使能开关 (HC_ENABLE/HC_DISABLE) */
    HC_HAL_UART_RxCallback_t rx_callback; /* 当 rx_irq_enable 为 HC_ENABLE 时必须提供有效回调 */
} HC_HAL_UART_InitCfg_t;

/*============================================================================
 *                        公开 API
 *===========================================================================*/

/**
 * @brief UART 模块初始化
 * 
 * 将句柄绑定到指定的硬件 ID，并根据配置参数完成外设时钟、引脚复用及通信格式的设置。
 * 
 * @param p_handle UART 句柄指针，输出参数
 * @param id 目标硬件实例 ID
 * @param p_cfg 初始化配置参数指针
 * @return HC_S32 成功返回 HC_HAL_OK，参数错误、实例不可用或底层未适配返回错误码。
 */
HC_S32 HC_HAL_UART_Init(HC_HAL_UART_Handle_t *p_handle, HC_HAL_UART_Id_e id, const HC_HAL_UART_InitCfg_t *p_cfg);

/**
 * @brief 阻塞式发送数据缓冲区
 * 
 * 当前模板仅供裸机参考；若迁移到 RTOS，请改为队列、中断或 DMA 驱动方案。
 * 
 * @param p_handle UART 句柄指针
 * @param p_buf 待发送的数据缓冲区指针
 * @param len 发送数据的字节长度
 * @return HC_S32 成功返回 HC_HAL_OK，错误或底层未适配返回错误码。
 */
HC_S32 HC_HAL_UART_Send(const HC_HAL_UART_Handle_t *p_handle, const HC_U8 *p_buf, HC_U16 len);

/**
 * @brief 阻塞式发送单字节数据
 * 
 * @param p_handle UART 句柄指针
 * @param byte 待发送的单字节数据
 * @return HC_S32 成功返回 HC_HAL_OK，底层未适配返回错误码。
 */
HC_S32 HC_HAL_UART_SendByte(const HC_HAL_UART_Handle_t *p_handle, HC_U8 byte);

/**
 * @brief UART 模块反初始化
 *
 * 这是通用 HAL 的反初始化模板接口。
 * 若底层平台的 UART 反初始化流程尚未接入，则返回 HC_ERR_NOT_READY，
 * 当前模板不会在未适配时假装反初始化成功，也不会提前清空软件状态。
 *
 * @param p_handle UART 句柄指针
 * @return HC_S32 成功返回 HC_HAL_OK，句柄无效、未初始化或底层未适配返回错误码。
 */
HC_S32 HC_HAL_UART_DeInit(HC_HAL_UART_Handle_t *p_handle);

/*============================================================================
 *                        中断处理入口
 *===========================================================================*/

/** @brief UART 实例 0 中断处理程序 */
HC_VOID HC_HAL_UART0_IRQHandler(HC_VOID);

/** @brief UART 实例 1 中断处理程序 */
HC_VOID HC_HAL_UART1_IRQHandler(HC_VOID);

#ifdef __cplusplus
}
#endif

#endif /* HC_HAL_UART_H */
