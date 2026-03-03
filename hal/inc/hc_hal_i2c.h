/**
 * @file    hc_hal_i2c.h
 * @brief   HC 团队 HAL I2C 对外接口
 */

#ifndef HC_HAL_I2C_H
#define HC_HAL_I2C_H

#include "hc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *                        宏定义与枚举
 *===========================================================================*/

/**
 * @brief I2C 硬件实例 ID 定义
 */
typedef enum {
    HC_HAL_I2C_ID_0 = 0, /* I2C 实例 0 (标称) */
    HC_HAL_I2C_ID_1 = 1, /* I2C 实例 1 (标称) */
    HC_HAL_I2C_ID_MAX    /* 实例 ID 最大值标记 */
} HC_HAL_I2C_Id_e;

/*============================================================================
 *                        数据结构定义
 *===========================================================================*/

/**
 * @brief I2C 控制句柄结构体
 */
typedef struct {
    HC_U8 instance; /* 绑定的硬件实例索引 (HC_HAL_I2C_Id_e) */
} HC_HAL_I2C_Handle_t;

/**
 * @brief I2C 初始化配置参数结构体
 */
typedef struct {
    HC_U32 bus_khz;    /* 总线频率 (单位：kHz，如 100 或 400) */
    HC_U32 timeout_ms; /* 阻塞传输的物理超时预算 (单位：ms) */
} HC_HAL_I2C_InitCfg_t;

/*============================================================================
 *                        公开 API
 *===========================================================================*/

/**
 * @brief I2C 模块初始化
 * 
 * 将句柄绑定到特定的硬件 I2C 控制器并初始化通信频率和基本传输时效。
 * 
 * @param p_handle I2C 句柄指针
 * @param id 目标硬件实例 ID
 * @param p_cfg 初始化配置参数
 * @return HC_S32 成功返回 HC_HAL_OK；若参数错误或底层硬件初始化未适配，返回对应错误码。
 */
HC_S32 HC_HAL_I2C_Init(HC_HAL_I2C_Handle_t *p_handle, HC_HAL_I2C_Id_e id, const HC_HAL_I2C_InitCfg_t *p_cfg);

/**
 * @brief I2C 主机写原始数据流
 * 
 * @param p_handle I2C 句柄指针
 * @param dev_addr 未左移的 7-bit 从机设备地址，合法范围 0x08~0x77
 * @param p_buf 待发送的数据缓冲区
 * @param len 待发送的字节数
 * @return HC_S32 成功返回 HC_HAL_OK，NACK、超时或底层未适配返回错误码。
 *
 * 说明：当前超时实现依赖 HC_HAL_SYSTICK_Init() 提供的 1ms 系统时间基准。
 */
HC_S32 HC_HAL_I2C_Write(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, const HC_U8 *p_buf, HC_U16 len);

/**
 * @brief I2C 主机读原始数据流
 * 
 * @param p_handle I2C 句柄指针
 * @param dev_addr 未左移的 7-bit 从机设备地址，合法范围 0x08~0x77
 * @param p_buf 存放读取数据的缓冲区
 * @param len 期望读取的字节数
 * @return HC_S32 成功返回 HC_HAL_OK，超时或底层未适配返回错误码。
 *
 * 说明：当前超时实现依赖 HC_HAL_SYSTICK_Init() 提供的 1ms 系统时间基准。
 */
HC_S32 HC_HAL_I2C_Read(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 *p_buf, HC_U16 len);

/**
 * @brief I2C 写入指定从机的单字节寄存器
 * 
 * @param p_handle I2C 句柄指针
 * @param dev_addr 未左移的 7-bit 从机设备地址，合法范围 0x08~0x77
 * @param reg 8-bit 寄存器地址
 * @param val 待写入的 8 位数值
 * @return HC_S32 成功返回 HC_HAL_OK，底层未适配返回错误码。
 *
 * 说明：当前模板只覆盖 8-bit 寄存器地址设备；16-bit 寄存器地址设备属于后续扩展 TODO。
 */
HC_S32 HC_HAL_I2C_WriteReg(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 reg, HC_U8 val);

/**
 * @brief I2C 读取指定从机的单字节寄存器
 * 
 * 执行标准时序：START -> ADDR(W) -> REG -> RE-START -> ADDR(R) -> DATA -> STOP
 * 
 * @param p_handle I2C 句柄指针
 * @param dev_addr 未左移的 7-bit 从机设备地址，合法范围 0x08~0x77
 * @param reg 8-bit 寄存器地址
 * @param p_val 存放读取结果的变量指针
 * @return HC_S32 成功返回 HC_HAL_OK，底层未适配返回错误码。
 *
 * 说明：当前模板只覆盖 8-bit 寄存器地址设备；16-bit 寄存器地址设备属于后续扩展 TODO。
 */
HC_S32 HC_HAL_I2C_ReadReg(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 reg, HC_U8 *p_val);

/**
 * @brief I2C 模块反初始化
 *
 * 这是通用 HAL 的反初始化模板接口。
 * 若底层平台的 I2C 反初始化流程尚未接入，则返回 HC_ERR_NOT_READY，
 * 当前接口不会在未适配时假装成功，也不会提前清除实例初始化状态。
 *
 * @param p_handle I2C 句柄指针
 * @return HC_S32 成功返回 HC_HAL_OK，句柄无效、未初始化或底层未适配返回错误码。
 */
HC_S32 HC_HAL_I2C_DeInit(HC_HAL_I2C_Handle_t *p_handle);

#ifdef __cplusplus
}
#endif

#endif /* HC_HAL_I2C_H */
