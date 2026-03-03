/**
 * @file    hc_hal_i2c.c
 * @brief   HC 团队 HAL I2C 模板实现
 */

/*============================================================================
 *                        包含头文件
 *===========================================================================*/
/* 本文件是主 HAL 模板层，不直接依赖任何厂商 SDK。 */
/* 移植到具体平台时，请在对应的 *_hw_* 私有函数中接入目标平台驱动调用。 */
#include "hc_hal_board_cfg.h"
#include "hc_hal_i2c.h"
#include "hc_hal_systick.h"

/*============================================================================
 *                        内部私有定义
 *===========================================================================*/

/**
 * @brief I2C 硬件资源映射结构体
 */
typedef struct {
    HC_VOID *base;     /* 外设基址或平台私有控制块指针 */
    HC_VOID *scl_port; /* SCL 端口句柄/端口标识 */
    HC_U32 scl_pin;    /* SCL 引脚编号或位掩码 */
    HC_U32 scl_iomux;  /* SCL 复用配置或平台私有选择值 */
    HC_VOID *sda_port; /* SDA 端口句柄/端口标识 */
    HC_U32 sda_pin;    /* SDA 引脚编号或位掩码 */
    HC_U32 sda_iomux;  /* SDA 复用配置或平台私有选择值 */
} I2cHwCfg_t;

/**
 * @brief I2C 运行期上下文状态结构体
 */
typedef struct {
    HC_Bool_e is_init;        /* 实例初始化状态标记 */
    HC_HAL_I2C_InitCfg_t cfg; /* 当前绑定的逻辑配置备份 */
} I2cCtx_t;

/*============================================================================
 *                        私有辅助函数声明
 *===========================================================================*/
static HC_Bool_e i2c_is_valid_id(HC_HAL_I2C_Id_e id);
static const I2cHwCfg_t *i2c_get_hw_cfg(HC_HAL_I2C_Id_e id);
static I2cCtx_t *i2c_get_ctx(HC_HAL_I2C_Id_e id);
static HC_S32 i2c_check_init_cfg(const HC_HAL_I2C_InitCfg_t *p_cfg);
static HC_S32 i2c_hw_init(const I2cHwCfg_t *p_hw, const HC_HAL_I2C_InitCfg_t *p_cfg);
static HC_S32 i2c_hw_deinit(const I2cHwCfg_t *p_hw);
static HC_S32 i2c_check_handle(const HC_HAL_I2C_Handle_t *p_handle);
static HC_S32 i2c_check_addr(HC_U8 dev_addr);
static HC_S32 i2c_check_transfer(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, const HC_U8 *p_buf, HC_U16 len);
static HC_S32 i2c_hw_get_bus_idle(const I2cHwCfg_t *p_hw, HC_Bool_e *p_is_idle);
static HC_S32 i2c_hw_write(const I2cHwCfg_t *p_hw, HC_U8 dev_addr, const HC_U8 *p_buf, HC_U16 len);
static HC_S32 i2c_hw_read(const I2cHwCfg_t *p_hw, HC_U8 dev_addr, HC_U8 *p_buf, HC_U16 len);
static HC_S32 i2c_hw_read_reg(const I2cHwCfg_t *p_hw, HC_U8 dev_addr, HC_U8 reg, HC_U8 *p_val);
static HC_S32 i2c_wait_idle(const I2cCtx_t *p_ctx, const I2cHwCfg_t *p_hw);
static HC_S32 i2c_write_then_read(const HC_HAL_I2C_Handle_t *p_handle,
                                  HC_U8 dev_addr,
                                  const HC_U8 *p_wr_buf,
                                  HC_U16 wr_len,
                                  HC_U8 *p_rd_buf,
                                  HC_U16 rd_len);

/*============================================================================
 *                        私有全局变量/配置表
 *===========================================================================*/

#if HC_HAL_I2C0_ENABLE
/** @brief I2C 实例 0 硬件配置表 */
static const I2cHwCfg_t s_i2c0_hw = {(HC_VOID *)HC_HAL_I2C0_BASE,
                                     HC_HAL_I2C0_SCL_PORT,
                                     HC_HAL_I2C0_SCL_PIN,
                                     HC_HAL_I2C0_SCL_IOMUX,
                                     HC_HAL_I2C0_SDA_PORT,
                                     HC_HAL_I2C0_SDA_PIN,
                                     HC_HAL_I2C0_SDA_IOMUX};
#endif

#if HC_HAL_I2C1_ENABLE
/** @brief I2C 实例 1 硬件配置表 */
static const I2cHwCfg_t s_i2c1_hw = {(HC_VOID *)HC_HAL_I2C1_BASE,
                                     HC_HAL_I2C1_SCL_PORT,
                                     HC_HAL_I2C1_SCL_PIN,
                                     HC_HAL_I2C1_SCL_IOMUX,
                                     HC_HAL_I2C1_SDA_PORT,
                                     HC_HAL_I2C1_SDA_PIN,
                                     HC_HAL_I2C1_SDA_IOMUX};
#endif

/** @brief 模块内部上下文数组 */
static I2cCtx_t s_i2c_ctx[HC_HAL_I2C_ID_MAX];

/*============================================================================
 *                        私有辅助函数实现
 *===========================================================================*/

/**
 * @brief 判断 I2C ID 是否在合法范围内
 * @param id 目标实例 ID
 * @return HC_TRUE 为有效
 */
static HC_Bool_e i2c_is_valid_id(HC_HAL_I2C_Id_e id)
{
    return (id < HC_HAL_I2C_ID_MAX) ? HC_TRUE : HC_FALSE;
}

/**
 * @brief 根据实例 ID 获取硬件配置描述符
 * @param id 目标实例 ID
 * @return 描述符指针或 HC_NULL_PTR
 */
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

/**
 * @brief 获取当前实例的上下文
 * @param id 目标实例 ID
 */
static I2cCtx_t *i2c_get_ctx(HC_HAL_I2C_Id_e id)
{
    if (i2c_is_valid_id(id) == HC_FALSE) {
        return HC_NULL_PTR;
    }

    return &s_i2c_ctx[(HC_U8)id];
}

/**
 * @brief 深度校验 I2C 初始化参数
 * @param p_cfg 配置指针
 */
static HC_S32 i2c_check_init_cfg(const HC_HAL_I2C_InitCfg_t *p_cfg)
{
    if (p_cfg == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (p_cfg->bus_khz == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    if (p_cfg->timeout_ms == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    return HC_HAL_OK;
}

static HC_S32 i2c_hw_init(const I2cHwCfg_t *p_hw, const HC_HAL_I2C_InitCfg_t *p_cfg)
{
    HC_UNUSED(p_hw);
    HC_UNUSED(p_cfg);

    /*
     * 待适配: 在此处填充 I2C 硬件初始化。
     * 建议流程:
     * 1. 使能外设时钟。
     * 2. 配置 SCL/SDA 引脚复用及开漏输出特性。
     * 3. 设置分频系数以匹配总线频率 (bus_khz)。
     * 4. 当前模板默认阻塞接口依赖 HC_HAL_SYSTICK_Init() 提供毫秒级超时基准。
     */
    return HC_ERR_NOT_READY;
}

/**
 * @brief 底层 I2C 反初始化入口
 *
 * 该函数是当前通用 HAL 模板中承接平台库/底层 API 反初始化调用的集中封装点。
 * 若迁移到具体芯片，请仅在此处接入目标平台的 I2C 关闭流程。
 *
 * @param p_hw 硬件配置描述符
 * @return HC_S32 成功返回 HC_HAL_OK，未适配返回 HC_ERR_NOT_READY
 */
static HC_S32 i2c_hw_deinit(const I2cHwCfg_t *p_hw)
{
    HC_UNUSED(p_hw);

    /*
     * 待适配: 在此处填充 I2C 硬件反初始化流程。
     * 建议替换流程:
     * 1. 关闭 I2C 中断源（若平台已启用）。
     * 2. 关闭 I2C 外设时钟以降低功耗。
     * 3. 将 SCL/SDA 引脚恢复为默认 GPIO / 高阻 / 输入状态。
     * 4. 清理控制器内部状态标志（若目标平台需要）。
     */
    return HC_ERR_NOT_READY;
}

static HC_S32 i2c_hw_get_bus_idle(const I2cHwCfg_t *p_hw, HC_Bool_e *p_is_idle)
{
    HC_UNUSED(p_hw);
    HC_UNUSED(p_is_idle);

    /*
     * 待适配: 在此处返回当前总线是否处于空闲状态。
     * 建议替换流程:
     * 1. 读取控制器 BUSY 标志。
     * 2. 空闲时将 *p_is_idle 置为 HC_TRUE。
     * 3. 忙碌时将 *p_is_idle 置为 HC_FALSE。
     */
    return HC_ERR_NOT_READY;
}

static HC_S32 i2c_hw_write(const I2cHwCfg_t *p_hw, HC_U8 dev_addr, const HC_U8 *p_buf, HC_U16 len)
{
    HC_UNUSED(p_hw);
    HC_UNUSED(dev_addr);
    HC_UNUSED(p_buf);
    HC_UNUSED(len);

    /*
     * 待适配: 在此处填充 I2C 主机写时序。
     * 建议替换流程:
     * 1. 发送未左移的 7-bit 地址与写控制位。
     * 2. 循环将缓冲区字节写入发送寄存器。
     * 3. 若迁移到 RTOS，请补互斥保护和基于 OS Tick 的超时。
     * 4. 若后续需要 10-bit 地址，请新增独立接口或扩展配置位，当前模板不支持。
     */
    return HC_ERR_NOT_READY;
}

static HC_S32 i2c_hw_read(const I2cHwCfg_t *p_hw, HC_U8 dev_addr, HC_U8 *p_buf, HC_U16 len)
{
    HC_UNUSED(p_hw);
    HC_UNUSED(dev_addr);
    HC_UNUSED(p_buf);
    HC_UNUSED(len);

    /*
     * 待适配: 在此处填充 I2C 主机读时序。
     * 建议替换流程:
     * 1. 发送未左移的 7-bit 地址与读控制位。
     * 2. 轮询接收标志并将数据写入 p_buf。
     * 3. 若迁移到 RTOS，请补互斥保护和基于 OS Tick 的超时。
     * 4. 若后续需要 10-bit 地址，请新增独立接口或扩展配置位，当前模板不支持。
     */
    return HC_ERR_NOT_READY;
}

static HC_S32 i2c_hw_read_reg(const I2cHwCfg_t *p_hw, HC_U8 dev_addr, HC_U8 reg, HC_U8 *p_val)
{
    HC_UNUSED(p_hw);
    HC_UNUSED(dev_addr);
    HC_UNUSED(reg);
    HC_UNUSED(p_val);

    /*
     * 待适配: 在此处填充 Repeated-START 的寄存器读事务。
     * 建议替换流程:
     * 1. 发送 START + ADDR(W)。
     * 2. 写入 8-bit 寄存器地址 reg。
     * 3. 发送 RE-START + ADDR(R)，中间不要发 STOP。
     * 4. 读取 1 字节数据至 *p_val。
     * 5. 发送 NACK + STOP 结束事务。
     * 6. 若后续需要 10-bit 地址，请新增独立接口或扩展配置位，当前模板不支持。
     */
    return HC_ERR_NOT_READY;
}

/**
 * @brief 校验 I2C 句柄及其绑定状态
 * @param p_handle 句柄指针
 */
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

/**
 * @brief 校验从机地址格式 (仅支持未左移的 7-bit 地址)
 * @param dev_addr 从机地址
 */
static HC_S32 i2c_check_addr(HC_U8 dev_addr)
{
    /*
     * 当前模板只接受 0x08~0x77：
     * 1. 明确拒绝带 R/W 位的 8-bit 地址写法（如 0xD0/0xD1）。
     * 2. 明确拒绝 7-bit 保留地址段 0x00~0x07 和 0x78~0x7F。
     * 3. 10-bit 地址不在本模板覆盖范围内，后续如需支持应单独扩展接口。
     */
    if ((dev_addr < 0x08u) || (dev_addr > 0x77u)) {
        return HC_HAL_ERR_INVALID;
    }

    return HC_HAL_OK;
}

/**
 * @brief 传输前的全量环境校验
 */
static HC_S32 i2c_check_transfer(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, const HC_U8 *p_buf, HC_U16 len)
{
    HC_S32 ret = i2c_check_handle(p_handle);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    ret = i2c_check_addr(dev_addr);
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
 * @brief 阻塞等待 I2C 总线进入空闲状态
 *
 * 前置条件：调用方已通过 i2c_check_handle / i2c_check_transfer 完成句柄校验，
 * 并一次性推导出 p_ctx 和 p_hw 指针传入，避免重复遍历 switch/数组。
 *
 * @param p_ctx 已校验的上下文指针
 * @param p_hw  已校验的硬件配置指针
 */
static HC_S32 i2c_wait_idle(const I2cCtx_t *p_ctx, const I2cHwCfg_t *p_hw)
{
    HC_U32 start_ms;
    HC_U32 now_ms;
    HC_S32 ret;

    if (p_ctx->cfg.timeout_ms == 0u) {
        return HC_HAL_ERR_INVALID;
    }

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

/**
 * @brief I2C 复合事务：写后读 (Repeated-START)
 *
 * 时序：START → ADDR(W) → p_wr_buf → RE-START → ADDR(R) → p_rd_buf → STOP
 * 中间不产生 STOP 条件，确保从机不会释放总线。
 *
 * @param p_handle  句柄指针
 * @param dev_addr  未左移的 7-bit 从机地址
 * @param p_wr_buf  写阶段数据缓冲区
 * @param wr_len    写阶段字节数
 * @param p_rd_buf  读阶段数据缓冲区
 * @param rd_len    读阶段字节数
 * @return HC_S32   成功返回 HC_HAL_OK，未适配或参数不匹配返回错误码
 */
static HC_S32 i2c_write_then_read(const HC_HAL_I2C_Handle_t *p_handle,
                                  HC_U8 dev_addr,
                                  const HC_U8 *p_wr_buf,
                                  HC_U16 wr_len,
                                  HC_U8 *p_rd_buf,
                                  HC_U16 rd_len)
{
    HC_S32 ret;
    I2cCtx_t *p_ctx;
    const I2cHwCfg_t *p_hw;

    ret = i2c_check_handle(p_handle);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    ret = i2c_check_addr(dev_addr);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    if ((p_wr_buf == HC_NULL_PTR) || (p_rd_buf == HC_NULL_PTR)) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if ((wr_len == 0u) || (rd_len == 0u)) {
        return HC_HAL_ERR_INVALID;
    }

    /* 一次推导，全程复用 */
    p_ctx = i2c_get_ctx((HC_HAL_I2C_Id_e)p_handle->instance);
    p_hw = i2c_get_hw_cfg((HC_HAL_I2C_Id_e)p_handle->instance);

    ret = i2c_wait_idle(p_ctx, p_hw);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    if ((wr_len != 1u) || (rd_len != 1u)) {
        return HC_ERR_NOT_READY;
    }

    return i2c_hw_read_reg(p_hw, dev_addr, p_wr_buf[0], p_rd_buf);
}

/*============================================================================
 *                        公开 API 函数实现
 *===========================================================================*/

HC_S32 HC_HAL_I2C_Init(HC_HAL_I2C_Handle_t *p_handle, HC_HAL_I2C_Id_e id, const HC_HAL_I2C_InitCfg_t *p_cfg)
{
    HC_S32 ret;
    I2cCtx_t *p_ctx;
    const I2cHwCfg_t *p_hw;

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

    p_hw = i2c_get_hw_cfg(id);
    if (p_hw == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    p_ctx = i2c_get_ctx(id);
    if (p_ctx == HC_NULL_PTR) {
        return HC_HAL_ERR_INVALID;
    }

    p_ctx->is_init = HC_FALSE;
    p_ctx->cfg = *p_cfg;

    ret = i2c_hw_init(p_hw, p_cfg);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    /* 仅在初始化成功后再向外导出实例号，避免失败时留下半初始化句柄。 */
    p_handle->instance = (HC_U8)id;
    p_ctx->is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_I2C_Write(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, const HC_U8 *p_buf, HC_U16 len)
{
    HC_S32 ret = i2c_check_transfer(p_handle, dev_addr, p_buf, len);
    I2cCtx_t *p_ctx;
    const I2cHwCfg_t *p_hw;

    if (ret != HC_HAL_OK) {
        return ret;
    }

    /* 一次推导，全程复用 */
    p_ctx = i2c_get_ctx((HC_HAL_I2C_Id_e)p_handle->instance);
    p_hw = i2c_get_hw_cfg((HC_HAL_I2C_Id_e)p_handle->instance);

    ret = i2c_wait_idle(p_ctx, p_hw);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    return i2c_hw_write(p_hw, dev_addr, p_buf, len);
}

HC_S32 HC_HAL_I2C_Read(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 *p_buf, HC_U16 len)
{
    HC_S32 ret = i2c_check_transfer(p_handle, dev_addr, p_buf, len);
    I2cCtx_t *p_ctx;
    const I2cHwCfg_t *p_hw;

    if (ret != HC_HAL_OK) {
        return ret;
    }

    /* 一次推导，全程复用 */
    p_ctx = i2c_get_ctx((HC_HAL_I2C_Id_e)p_handle->instance);
    p_hw = i2c_get_hw_cfg((HC_HAL_I2C_Id_e)p_handle->instance);

    ret = i2c_wait_idle(p_ctx, p_hw);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    return i2c_hw_read(p_hw, dev_addr, p_buf, len);
}

HC_S32 HC_HAL_I2C_WriteReg(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 reg, HC_U8 val)
{
    HC_U8 tx_buf[2];

    /* 当前模板只覆盖 8-bit 寄存器地址；如需 16-bit 寄存器地址，请扩展独立接口。 */
    tx_buf[0] = reg;
    tx_buf[1] = val;

    return HC_HAL_I2C_Write(p_handle, dev_addr, tx_buf, (HC_U16)HC_ARRAY_SIZE(tx_buf));
}

HC_S32 HC_HAL_I2C_ReadReg(const HC_HAL_I2C_Handle_t *p_handle, HC_U8 dev_addr, HC_U8 reg, HC_U8 *p_val)
{
    if (p_val == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    /* 当前模板只覆盖 8-bit 寄存器地址，并使用标准 Repeated-START 流程避免提前释放总线。 */
    return i2c_write_then_read(p_handle, dev_addr, &reg, 1u, p_val, 1u);
}

HC_S32 HC_HAL_I2C_DeInit(HC_HAL_I2C_Handle_t *p_handle)
{
    HC_S32 ret;
    I2cCtx_t *p_ctx;
    const I2cHwCfg_t *p_hw;

    if (p_handle == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    ret = i2c_check_handle(p_handle);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx = i2c_get_ctx((HC_HAL_I2C_Id_e)p_handle->instance);
    p_hw = i2c_get_hw_cfg((HC_HAL_I2C_Id_e)p_handle->instance);
    if ((p_ctx == HC_NULL_PTR) || (p_hw == HC_NULL_PTR)) {
        return HC_HAL_ERR_INVALID;
    }

    /* 通用 HAL 层只负责组织反初始化流程，平台相关寄存器/SDK 调用统一下沉到底层 helper。 */
    ret = i2c_hw_deinit(p_hw);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    p_ctx->is_init = HC_FALSE;
    return HC_HAL_OK;
}
