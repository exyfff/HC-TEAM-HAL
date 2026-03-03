/**
 * @file    hc_hal_example_mspm0_wdg.c
 * @brief   MSPM0 平台的 HC HAL 看门狗参考实现。
 *
 * 本文件将通用 HC HAL 看门狗接口映射到 TI MSPM0 DriverLib。
 */

#include "ti_dl_lib.h"
#include "hc_hal_example_mspm0_board_cfg.h"
#include "hc_hal_wdg.h"

static HC_Bool_e s_wdg_is_init = HC_FALSE;
static HC_Bool_e s_wdg_reset_flag = HC_FALSE;
static HC_Bool_e s_wdg_reset_flag_locked = HC_FALSE;

static HC_S32 wdg_check_init(HC_VOID)
{
    return (s_wdg_is_init == HC_TRUE) ? HC_HAL_OK : HC_HAL_ERR_NOT_INIT;
}

static HC_S32 wdg_hw_init(HC_VOID)
{
    if (HC_HAL_WDG_RESET_ENABLE != HC_TRUE) {
        return HC_HAL_ERR_NOT_PERM;
    }

    if (DL_IWDT_isWatchDogRunning(HC_HAL_EXAMPLE_MSPM0_WDG_BASE)) {
        return HC_HAL_OK;
    }

    DL_IWDT_setClockDivider(HC_HAL_EXAMPLE_MSPM0_WDG_BASE, HC_HAL_EXAMPLE_MSPM0_WDG_CLOCK_DIVIDER);
    DL_IWDT_setTimerPeriod(HC_HAL_EXAMPLE_MSPM0_WDG_BASE, HC_HAL_EXAMPLE_MSPM0_WDG_TIMER_PERIOD);
    DL_IWDT_enableModule(HC_HAL_EXAMPLE_MSPM0_WDG_BASE);
    DL_IWDT_restart(HC_HAL_EXAMPLE_MSPM0_WDG_BASE);
    return HC_HAL_OK;
}

static HC_S32 wdg_hw_feed(HC_VOID)
{
    DL_IWDT_restart(HC_HAL_EXAMPLE_MSPM0_WDG_BASE);
    return HC_HAL_OK;
}

static HC_S32 wdg_hw_read_reset_flag(HC_Bool_e *p_is_wdg_reset)
{
    DL_SYSCTL_RESET_CAUSE cause;

    if (p_is_wdg_reset == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    *p_is_wdg_reset = HC_FALSE;
    cause = DL_SYSCTL_getResetCause();

    /*
     * MSPM0 IWDT 超时触发 SoC 级 POR 复位（非 WWDT violation）。
     * 复位判断策略：
     *
     * 1. WWDT violation — 直接匹配。
     * 2. POR/BOR — IWDT 超时会触发此路径。但 POR 也可能由电源上电
     *    或 BOR 触发，因此这里是"宽松匹配"：只要是 POR/BOR 就标记
     *    为可能的 WDG 复位。
     *
     * 注意：冷启动后 IWDT 尚未启动，DL_IWDT_isWatchDogRunning() 会
     * 返回 false，不能用来过滤 POR 路径。如需精确区分 IWDT 触发的
     * POR 与电源上电 POR，需借助 RSTCTL 寄存器或非易失存储。
     */
    if (cause == DL_SYSCTL_RESET_CAUSE_SYSRST_WWDT0_VIOLATION) {
        *p_is_wdg_reset = HC_TRUE;
    }
#ifdef DL_SYSCTL_RESET_CAUSE_SYSRST_WWDT1_VIOLATION
    else if (cause == DL_SYSCTL_RESET_CAUSE_SYSRST_WWDT1_VIOLATION) {
        *p_is_wdg_reset = HC_TRUE;
    }
#endif
    else if (cause == DL_SYSCTL_RESET_CAUSE_POR_BOR_WAKE_FROM_SHUTDOWN) {
        *p_is_wdg_reset = HC_TRUE;
    }

    return HC_HAL_OK;
}

static HC_S32 wdg_hw_clear_reset_flag(HC_VOID)
{
    return HC_HAL_OK;
}

HC_S32 HC_HAL_WDG_Init(HC_VOID)
{
    HC_S32 ret;

    if (s_wdg_is_init == HC_TRUE) {
        return HC_HAL_ERR_ALREADY_INIT;
    }

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

    *p_is_wdg_reset = s_wdg_reset_flag;
    return HC_HAL_OK;
}
