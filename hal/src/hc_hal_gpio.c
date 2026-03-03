/**
 * @file    hc_hal_gpio.c
 * @brief   HC 团队 HAL GPIO 模板实现
 *
 * 本文件是通用模板层，仅保留公共接口骨架与参数校验。
 * 具体平台的 GPIO 映射、寄存器访问和中断控制请在平台实现中接入。
 */

#include "hc_hal_gpio.h"

/*============================================================================
 *                        私有全局变量
 *===========================================================================*/

/** @brief GPIO 模块初始化状态；当前模板未接入硬件，因此始终保持未就绪 */
static HC_Bool_e s_gpio_is_init = HC_FALSE;

/*============================================================================
 *                        私有辅助函数
 *===========================================================================*/

/**
 * @brief 判断虚拟引脚号是否合法
 * @param vpin 虚拟引脚索引
 * @return HC_TRUE 为合法，否则为 HC_FALSE
 */
static HC_Bool_e gpio_is_valid_vpin(HC_HAL_GPIO_VPin_e vpin)
{
    return (((HC_U32)vpin) < (HC_U32)VPIN_MAX) ? HC_TRUE : HC_FALSE;
}

/*============================================================================
 *                        公开 API 函数实现
 *===========================================================================*/

HC_S32 HC_HAL_GPIO_Init(HC_VOID)
{
    /*
     * 待适配: 在此处接入目标平台的 GPIO 初始化。
     * 建议替换流程:
     * 1. 建立虚拟引脚到物理引脚的映射表。
     * 2. 配置方向、默认电平、上下拉和中断触发方式。
     * 3. 在硬件初始化完成后将 s_gpio_is_init 置为 HC_TRUE。
     */
    s_gpio_is_init = HC_FALSE;
    return HC_ERR_NOT_READY;
}

HC_S32 HC_HAL_GPIO_SetPin(HC_HAL_GPIO_VPin_e vpin)
{
    if (gpio_is_valid_vpin(vpin) == HC_FALSE) {
        return HC_HAL_ERR_INVALID;
    }

    HC_UNUSED(vpin);

    if (s_gpio_is_init == HC_FALSE) {
        return HC_ERR_NOT_READY;
    }

    /*
     * 待适配: 将 vpin 映射到目标平台的物理输出脚并输出高电平。
     */
    return HC_ERR_NOT_READY;
}

HC_S32 HC_HAL_GPIO_ResetPin(HC_HAL_GPIO_VPin_e vpin)
{
    if (gpio_is_valid_vpin(vpin) == HC_FALSE) {
        return HC_HAL_ERR_INVALID;
    }

    HC_UNUSED(vpin);

    if (s_gpio_is_init == HC_FALSE) {
        return HC_ERR_NOT_READY;
    }

    /*
     * 待适配: 将 vpin 映射到目标平台的物理输出脚并输出低电平。
     */
    return HC_ERR_NOT_READY;
}

HC_S32 HC_HAL_GPIO_TogglePin(HC_HAL_GPIO_VPin_e vpin)
{
    if (gpio_is_valid_vpin(vpin) == HC_FALSE) {
        return HC_HAL_ERR_INVALID;
    }

    HC_UNUSED(vpin);

    if (s_gpio_is_init == HC_FALSE) {
        return HC_ERR_NOT_READY;
    }

    /*
     * 待适配: 将 vpin 映射到目标平台的物理输出脚并翻转电平。
     */
    return HC_ERR_NOT_READY;
}

HC_S32 HC_HAL_GPIO_ReadPin(HC_HAL_GPIO_VPin_e vpin, HC_HAL_GPIO_PinState_e *p_state)
{
    if (p_state == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    if (gpio_is_valid_vpin(vpin) == HC_FALSE) {
        return HC_HAL_ERR_INVALID;
    }

    HC_UNUSED(vpin);

    if (s_gpio_is_init == HC_FALSE) {
        return HC_ERR_NOT_READY;
    }

    /*
     * 待适配: 读取目标平台物理引脚电平，并映射为 HC_PIN_SET / HC_PIN_RESET。
     * 当前模板不写回 *p_state，避免在未适配状态下返回误导性默认值。
     */
    return HC_ERR_NOT_READY;
}

/*============================================================================
 *                        中断转发与处理实现
 *===========================================================================*/

HC_VOID HC_HAL_GPIOA_IRQHandler(HC_VOID)
{
    /*
     * 待适配: 读取端口 A 的中断状态，清标志并将事件转发到 HC_HAL_GPIO_Callback()。
     * 当前模板仅保留符号，避免启动文件引用缺失。
     */
    if (s_gpio_is_init == HC_FALSE) {
        return;
    }
}

HC_VOID HC_HAL_GPIOB_IRQHandler(HC_VOID)
{
    /*
     * 待适配: 读取端口 B 的中断状态，清标志并将事件转发到 HC_HAL_GPIO_Callback()。
     * 当前模板仅保留符号，避免启动文件引用缺失。
     */
    if (s_gpio_is_init == HC_FALSE) {
        return;
    }
}

HC_WEAK HC_VOID HC_HAL_GPIO_Callback(HC_HAL_GPIO_VPin_e vpin)
{
    HC_UNUSED(vpin);
}
