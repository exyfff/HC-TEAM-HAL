/**
 * @file    hc_hal_example_stm32_wdg.c
 * @brief   HC 团队 HAL WDG 的 STM32 参考实现
 *
 * 本文件使用标准 STM32 HAL 的独立看门狗 (IWDG) 接口示例。
 */

#include "stm32xxxx_hal.h"
#include "hc_hal_example_stm32_board_cfg.h"
#include "hc_hal_wdg.h"

static HC_Bool_e s_wdg_is_init = HC_FALSE;
static HC_Bool_e s_wdg_reset_flag = HC_FALSE;
static HC_Bool_e s_wdg_reset_flag_locked = HC_FALSE;
static IWDG_HandleTypeDef s_iwdg_handle;

static HC_S32 wdg_check_init(HC_VOID)
{
    return (s_wdg_is_init == HC_TRUE) ? HC_HAL_OK : HC_HAL_ERR_NOT_INIT;
}

static HC_S32 wdg_hw_init(HC_VOID)
{
    s_iwdg_handle.Instance = IWDG;
    s_iwdg_handle.Init.Prescaler = HC_HAL_EXAMPLE_STM32_WDG_PRESCALER;
    s_iwdg_handle.Init.Reload = HC_HAL_EXAMPLE_STM32_WDG_RELOAD;

    if (HAL_IWDG_Init(&s_iwdg_handle) != HAL_OK) {
        return HC_ERR_NOT_READY;
    }

    return HC_HAL_OK;
}

static HC_S32 wdg_hw_feed(HC_VOID)
{
    if (HAL_IWDG_Refresh(&s_iwdg_handle) != HAL_OK) {
        return HC_ERR_NOT_READY;
    }

    return HC_HAL_OK;
}

static HC_S32 wdg_hw_read_reset_flag(HC_Bool_e *p_is_wdg_reset)
{
    if (p_is_wdg_reset == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    *p_is_wdg_reset = (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET) ? HC_TRUE : HC_FALSE;
    return HC_HAL_OK;
}

static HC_S32 wdg_hw_clear_reset_flag(HC_VOID)
{
    __HAL_RCC_CLEAR_RESET_FLAGS();
    return HC_HAL_OK;
}

HC_S32 HC_HAL_WDG_Init(HC_VOID)
{
    HC_S32 ret;

    if (s_wdg_is_init == HC_TRUE) {
        return HC_HAL_ERR_ALREADY_INIT;
    }

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
