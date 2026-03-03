/**
 * @file    hc_hal_wdg.c
 * @brief   HC 团队 HAL 看门狗模板实现
 */

/*============================================================================
 *                        包含头文件
 *===========================================================================*/
/* 本文件是主 HAL 模板层，不直接依赖任何厂商 SDK。 */
/* 移植到具体平台时，请在本模块的底层 helper 中替换为目标平台看门狗 API。 */
#include "hc_hal_board_cfg.h"
#include "hc_hal_wdg.h"

/*============================================================================
 *                        内部私有定义
 *===========================================================================*/

/**
 * @brief 看门狗内部硬件配置结构体 (单实例示例)
 */
typedef struct {
    HC_U32 timeout_ms;      /* 超时时间阈值 (单位：ms) */
    HC_Bool_e reset_enable; /* 超时后是否触发全局复位 (HC_TRUE/HC_FALSE) */
} WdgCfg_t;

/*============================================================================
 *                        私有辅助函数声明
 *===========================================================================*/
static HC_S32 wdg_check_init(HC_VOID);
static HC_S32 wdg_hw_init(HC_VOID);
static HC_S32 wdg_hw_feed(HC_VOID);
static HC_S32 wdg_hw_read_reset_flag(HC_Bool_e *p_is_wdg_reset);
static HC_S32 wdg_hw_clear_reset_flag(HC_VOID);

/*============================================================================
 *                        私有全局变量/配置表
 *===========================================================================*/

/** @brief 看门狗静态配置参数（取自 hc_hal_board_cfg.h） */
static const WdgCfg_t s_wdg_cfg = {
    HC_HAL_WDG_TIMEOUT_MS,
    HC_HAL_WDG_RESET_ENABLE
};

/** @brief 模块初始化状态标记 */
static HC_Bool_e s_wdg_is_init = HC_FALSE;

/** @brief 看门狗复位来源标记缓存 */
static HC_Bool_e s_wdg_reset_flag = HC_FALSE;

/** @brief 复位来源锁定标记：只允许在冷启动后的第一次 Init 中读取一次 */
static HC_Bool_e s_wdg_reset_flag_locked = HC_FALSE;

/*============================================================================
 *                        私有辅助函数实现
 *===========================================================================*/

/**
 * @brief 内部校验看门狗模块是否已初始化
 * @return HC_HAL_OK 为就绪状态
 */
static HC_S32 wdg_check_init(HC_VOID)
{
    if (s_wdg_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    return HC_HAL_OK;
}

static HC_S32 wdg_hw_init(HC_VOID)
{
    /*
     * 待适配: 在此处插入具体芯片的看门狗配置时序。
     * 替换流程:
     * 1. 根据 s_wdg_cfg 配置喂狗时间窗口。
     * 2. 开启看门狗计数器。
     */
    return HC_ERR_NOT_READY;
}

static HC_S32 wdg_hw_feed(HC_VOID)
{
    /*
     * 待适配: 写入目标芯片的喂狗/重置指令。
     * 通常包含：
     * 1. 向控制寄存器写入特定的密钥(Key)解锁。
     * 2. 执行喂狗刷新指令。
     */
    return HC_ERR_NOT_READY;
}

static HC_S32 wdg_hw_read_reset_flag(HC_Bool_e *p_is_wdg_reset)
{
    HC_UNUSED(p_is_wdg_reset);

    /*
     * 待适配: 仅在冷启动后的第一次 Init 中读取复位来源。
     * 替换流程:
     * 1. 查询硬件复位状态寄存器，只提取“是否由看门狗触发复位”的标志。
     * 2. 将结果写入 *p_is_wdg_reset。
     */
    return HC_ERR_NOT_READY;
}

static HC_S32 wdg_hw_clear_reset_flag(HC_VOID)
{
    /*
     * 待适配: 清空硬件复位状态寄存器中的看门狗复位标志，避免下次冷启动误判。
     */
    return HC_ERR_NOT_READY;
}

/*============================================================================
 *                        公开 API 函数实现
 *===========================================================================*/

HC_S32 HC_HAL_WDG_Init(HC_VOID)
{
    HC_S32 ret;

    /* 看门狗一旦启动不可停止，禁止重复初始化以防配置被意外覆盖 */
    if (s_wdg_is_init == HC_TRUE) {
        return HC_HAL_ERR_ALREADY_INIT;
    }

    /* 状态同步：重置初始化标记 */
    s_wdg_is_init = HC_FALSE;

    if (s_wdg_reset_flag_locked == HC_FALSE) {
        ret = wdg_hw_read_reset_flag(&s_wdg_reset_flag);
        if (ret != HC_HAL_OK) {
            return ret;
        }

        ret = wdg_hw_clear_reset_flag();
        if (ret != HC_HAL_OK) {
            return ret;
        }

        s_wdg_reset_flag_locked = HC_TRUE;
    }

    HC_UNUSED(s_wdg_cfg.timeout_ms);
    HC_UNUSED(s_wdg_cfg.reset_enable);

    ret = wdg_hw_init();
    if (ret != HC_HAL_OK) {
        return ret;
    }

    s_wdg_is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_WDG_Feed(HC_VOID)
{
    HC_S32 ret = wdg_check_init();

    if (ret != HC_HAL_OK) {
        return ret;
    }

    return wdg_hw_feed();
}

HC_S32 HC_HAL_WDG_GetResetFlag(HC_Bool_e *p_is_wdg_reset)
{
    HC_S32 ret;

    if (p_is_wdg_reset == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    ret = wdg_check_init();
    if (ret != HC_HAL_OK) {
        return ret;
    }

    /*
     * 访问策略：上层只读取冷启动后第一次 Init 时锁定的缓存结果，
     * 不在运行期重复读取硬件寄存器，避免状态被后续初始化覆盖。
     */
    *p_is_wdg_reset = s_wdg_reset_flag;
    return HC_HAL_OK;
}
