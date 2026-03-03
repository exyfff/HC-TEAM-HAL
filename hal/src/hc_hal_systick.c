/**
 * @file    hc_hal_systick.c
 * @brief   HC 团队 HAL SYSTICK 模板实现
 */

/*============================================================================
 *                        包含头文件
 *===========================================================================*/
/* 本文件是主 HAL 模板层，不直接依赖任何厂商 SDK。 */
/* 移植到具体平台时，请在本模块的底层时基接入点替换为目标平台 API。 */
#include "hc_hal_board_cfg.h"
#include "hc_hal_systick.h"

/*============================================================================
 *                        私有全局变量
 *===========================================================================*/

/** @brief 模块初始化状态标记；ISR 中读取，主线程写入，必须 volatile */
static volatile HC_Bool_e s_systick_is_init = HC_FALSE;

/** @brief 系统毫秒节拍计数；在中断与主线代码之间共享，必须使用 volatile */
static volatile HC_U32 s_systick_tick_ms = 0u;

/*============================================================================
 *                        私有辅助函数声明
 *===========================================================================*/
static HC_S32 systick_hw_init(HC_VOID);

/*============================================================================
 *                        私有辅助函数实现
 *===========================================================================*/

static HC_S32 systick_hw_init(HC_VOID)
{
    /*
     * 待适配: 在此处接入目标芯片的 SysTick 初始化。
     * 建议替换流程:
     * 1. 以 (HC_HAL_SYSTICK_CPU_HZ / HC_HAL_SYSTICK_TICK_HZ) 计算重装值。
     * 2. 使用 SysTick_Config() 或等效寄存器流程打开 1ms 节拍中断。
     * 3. 若 RTOS 已接管 SysTick，请不要直接复用本实现，而应改为接入 OS Tick。
     */
    return HC_ERR_NOT_READY;
}

/*============================================================================
 *                        公开 API 函数实现
 *===========================================================================*/

HC_S32 HC_HAL_SYSTICK_Init(HC_VOID)
{
    HC_S32 ret;

    /* 系统节拍为全局基准服务，运行期不允许重配；重复调用会清零 tick 计数器，破坏依赖方的超时计算 */
    if (s_systick_is_init == HC_TRUE) {
        return HC_HAL_ERR_ALREADY_INIT;
    }

    if (HC_HAL_SYSTICK_TICK_HZ != 1000u) {
        return HC_HAL_ERR_INVALID;
    }

    if (HC_HAL_SYSTICK_CPU_HZ == 0u) {
        return HC_HAL_ERR_INVALID;
    }

    if (HC_HAL_SYSTICK_CPU_HZ < HC_HAL_SYSTICK_TICK_HZ) {
        return HC_HAL_ERR_INVALID;
    }

    s_systick_is_init = HC_FALSE;
    s_systick_tick_ms = 0u;

    ret = systick_hw_init();
    if (ret != HC_HAL_OK) {
        return ret;
    }

    s_systick_is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_SYSTICK_GetTickMs(HC_U32 *p_tick_ms)
{
    if (p_tick_ms == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (s_systick_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    *p_tick_ms = s_systick_tick_ms;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_SYSTICK_DelayMs(HC_U32 delay_ms)
{
    HC_U32 start_ms;
    HC_U32 now_ms;
    HC_S32 ret;

    if (delay_ms == 0u) {
        return HC_HAL_OK;
    }

    ret = HC_HAL_SYSTICK_GetTickMs(&start_ms);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    /*
     * 当前是裸机忙等模板。
     * 若迁移到 RTOS，请改为 OS 提供的任务延时接口，而不是在任务上下文中持续自旋。
     */
    do {
        ret = HC_HAL_SYSTICK_GetTickMs(&now_ms);
        if (ret != HC_HAL_OK) {
            return ret;
        }
    } while ((now_ms - start_ms) < delay_ms);

    return HC_HAL_OK;
}

HC_S32 HC_HAL_SYSTICK_DelayUs(HC_U32 delay_us)
{
    HC_U32 loop_per_us;

    if (delay_us == 0u) {
        return HC_HAL_OK;
    }

    if (s_systick_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    /*
     * 当前是裸机近似忙等模板。
     * 若对精度要求较高，或系统已迁移到 RTOS，请改用专用定时器/DWT/高精度计数器。
     */
    loop_per_us = HC_HAL_SYSTICK_CPU_HZ /
                  (HC_HAL_SYSTICK_DELAY_US_CYCLES * 1000000u);
    if (loop_per_us == 0u) {
        loop_per_us = 1u;
    }

    while (delay_us > 0u) {
        volatile HC_U32 loop_cnt = loop_per_us;

        while (loop_cnt > 0u) {
            loop_cnt--;
        }

        delay_us--;
    }

    return HC_HAL_OK;
}

HC_VOID HC_HAL_SYSTICK_IRQHandler(HC_VOID)
{
    if (s_systick_is_init == HC_FALSE) {
        return;
    }

    s_systick_tick_ms++;
}
