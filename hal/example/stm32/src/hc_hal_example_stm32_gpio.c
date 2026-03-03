/**
 * @file    hc_hal_example_stm32_gpio.c
 * @brief   HC 团队 HAL GPIO 的 STM32 参考实现
 *
 * 本文件展示如何用标准 STM32 HAL API 实现主 HAL 的 GPIO 接口。
 * 由于主公共头中的 IRQ 入口名仍是统一命名，实际工程中建议由芯片启动文件
 * 将真实 EXTI 向量转发到本文件中的兼容入口，或直接调用 HAL_GPIO_EXTI_IRQHandler()。
 */

#include "stm32xxxx_hal.h"
#include "hc_hal_example_stm32_board_cfg.h"
#include "hc_hal_gpio.h"

typedef enum {
    HC_EXAMPLE_GPIO_DIR_IN = 0,
    HC_EXAMPLE_GPIO_DIR_OUT = 1
} HcExampleGpioDir_e;

typedef struct {
    GPIO_TypeDef *port;
    HC_U16 pin;
    HcExampleGpioDir_e dir;
    HC_U32 pull;
    HC_HAL_GPIO_PinState_e init_level;
    IRQn_Type irqn;
    HC_Bool_e use_irq;
} HcExampleGpioCfg_t;

static const HcExampleGpioCfg_t s_gpio_map[VPIN_MAX] = {
    {HC_HAL_EXAMPLE_STM32_VPIN_LED_1_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_LED_1_PIN,
     HC_EXAMPLE_GPIO_DIR_OUT,
     GPIO_NOPULL,
     HC_PIN_RESET,
     EXTI0_IRQn,
     HC_FALSE},
    {HC_HAL_EXAMPLE_STM32_VPIN_LED_2_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_LED_2_PIN,
     HC_EXAMPLE_GPIO_DIR_OUT,
     GPIO_NOPULL,
     HC_PIN_RESET,
     EXTI1_IRQn,
     HC_FALSE},
    {HC_HAL_EXAMPLE_STM32_VPIN_LED_3_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_LED_3_PIN,
     HC_EXAMPLE_GPIO_DIR_OUT,
     GPIO_NOPULL,
     HC_PIN_RESET,
     EXTI2_IRQn,
     HC_FALSE},
    {HC_HAL_EXAMPLE_STM32_VPIN_RELAY_1_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_RELAY_1_PIN,
     HC_EXAMPLE_GPIO_DIR_OUT,
     GPIO_NOPULL,
     HC_PIN_RESET,
     EXTI0_IRQn,
     HC_FALSE},
    {HC_HAL_EXAMPLE_STM32_VPIN_RELAY_2_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_RELAY_2_PIN,
     HC_EXAMPLE_GPIO_DIR_OUT,
     GPIO_NOPULL,
     HC_PIN_RESET,
     EXTI1_IRQn,
     HC_FALSE},
    {HC_HAL_EXAMPLE_STM32_VPIN_BUZZER_EN_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_BUZZER_EN_PIN,
     HC_EXAMPLE_GPIO_DIR_OUT,
     GPIO_NOPULL,
     HC_PIN_RESET,
     EXTI2_IRQn,
     HC_FALSE},
    {HC_HAL_EXAMPLE_STM32_VPIN_MOTOR_EN_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_MOTOR_EN_PIN,
     HC_EXAMPLE_GPIO_DIR_OUT,
     GPIO_NOPULL,
     HC_PIN_RESET,
     EXTI15_10_IRQn,
     HC_FALSE},
    {HC_HAL_EXAMPLE_STM32_VPIN_SENSOR_PWR_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_SENSOR_PWR_PIN,
     HC_EXAMPLE_GPIO_DIR_OUT,
     GPIO_NOPULL,
     HC_PIN_RESET,
     EXTI15_10_IRQn,
     HC_FALSE},
    {HC_HAL_EXAMPLE_STM32_VPIN_SPI_CS_0_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_SPI_CS_0_PIN,
     HC_EXAMPLE_GPIO_DIR_OUT,
     GPIO_NOPULL,
     HC_PIN_RESET,
     EXTI15_10_IRQn,
     HC_FALSE},
    {HC_HAL_EXAMPLE_STM32_VPIN_KEY_1_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_KEY_1_PIN,
     HC_EXAMPLE_GPIO_DIR_IN,
     GPIO_PULLUP,
     HC_PIN_RESET,
     HC_HAL_EXAMPLE_STM32_VPIN_KEY_1_IRQn,
     HC_TRUE},
    {HC_HAL_EXAMPLE_STM32_VPIN_KEY_2_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_KEY_2_PIN,
     HC_EXAMPLE_GPIO_DIR_IN,
     GPIO_PULLUP,
     HC_PIN_RESET,
     HC_HAL_EXAMPLE_STM32_VPIN_KEY_2_IRQn,
     HC_TRUE},
    {HC_HAL_EXAMPLE_STM32_VPIN_EXTI_1_PORT,
     HC_HAL_EXAMPLE_STM32_VPIN_EXTI_1_PIN,
     HC_EXAMPLE_GPIO_DIR_IN,
     GPIO_PULLUP,
     HC_PIN_RESET,
     HC_HAL_EXAMPLE_STM32_VPIN_EXTI_1_IRQn,
     HC_TRUE},
};

static HC_Bool_e s_gpio_is_init = HC_FALSE;

static HC_Bool_e gpio_is_valid_vpin(HC_HAL_GPIO_VPin_e vpin)
{
    return (((HC_U32)vpin) < (HC_U32)VPIN_MAX) ? HC_TRUE : HC_FALSE;
}

static HC_S32 gpio_check_vpin(HC_HAL_GPIO_VPin_e vpin)
{
    if (gpio_is_valid_vpin(vpin) == HC_FALSE) {
        return HC_HAL_ERR_INVALID;
    }

    if (s_gpio_is_init == HC_FALSE) {
        return HC_HAL_ERR_NOT_INIT;
    }

    return HC_HAL_OK;
}

static HC_S32 gpio_check_output_vpin(HC_HAL_GPIO_VPin_e vpin)
{
    HC_S32 ret = gpio_check_vpin(vpin);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    if (s_gpio_map[vpin].dir != HC_EXAMPLE_GPIO_DIR_OUT) {
        return HC_HAL_ERR_NOT_PERM;
    }

    return HC_HAL_OK;
}

static HC_VOID gpio_apply_cfg(const HcExampleGpioCfg_t *p_cfg)
{
    GPIO_InitTypeDef gpio_cfg;

    if (p_cfg->dir == HC_EXAMPLE_GPIO_DIR_OUT) {
        gpio_cfg.Mode = GPIO_MODE_OUTPUT_PP;
    } else {
        gpio_cfg.Mode = p_cfg->use_irq ? GPIO_MODE_IT_FALLING : GPIO_MODE_INPUT;
    }

    gpio_cfg.Pin = p_cfg->pin;
    gpio_cfg.Pull = p_cfg->pull;
    gpio_cfg.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_cfg.Alternate = 0u;

    HAL_GPIO_Init(p_cfg->port, &gpio_cfg);

    /* 先 Init 后 WritePin，避免 GPIO 模式切换期间的电平毛刺窗口 */
    if (p_cfg->dir == HC_EXAMPLE_GPIO_DIR_OUT) {
        HAL_GPIO_WritePin(p_cfg->port, p_cfg->pin, (p_cfg->init_level == HC_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    if (p_cfg->use_irq == HC_TRUE) {
        HAL_NVIC_SetPriority(p_cfg->irqn, 5u, 0u);
        HAL_NVIC_EnableIRQ(p_cfg->irqn);
    }
}

HC_S32 HC_HAL_GPIO_Init(HC_VOID)
{
    HC_U8 i;

    if (s_gpio_is_init == HC_TRUE) {
        return HC_HAL_ERR_ALREADY_INIT;
    }

    HC_HAL_EXAMPLE_STM32_GPIOA_CLK_ENABLE();
    HC_HAL_EXAMPLE_STM32_GPIOB_CLK_ENABLE();
    HC_HAL_EXAMPLE_STM32_GPIOC_CLK_ENABLE();

    for (i = 0u; i < (HC_U8)VPIN_MAX; i++) {
        gpio_apply_cfg(&s_gpio_map[i]);
    }

    s_gpio_is_init = HC_TRUE;
    return HC_HAL_OK;
}

HC_S32 HC_HAL_GPIO_SetPin(HC_HAL_GPIO_VPin_e vpin)
{
    HC_S32 ret = gpio_check_output_vpin(vpin);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    HAL_GPIO_WritePin(s_gpio_map[vpin].port, s_gpio_map[vpin].pin, GPIO_PIN_SET);
    return HC_HAL_OK;
}

HC_S32 HC_HAL_GPIO_ResetPin(HC_HAL_GPIO_VPin_e vpin)
{
    HC_S32 ret = gpio_check_output_vpin(vpin);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    HAL_GPIO_WritePin(s_gpio_map[vpin].port, s_gpio_map[vpin].pin, GPIO_PIN_RESET);
    return HC_HAL_OK;
}

HC_S32 HC_HAL_GPIO_TogglePin(HC_HAL_GPIO_VPin_e vpin)
{
    HC_S32 ret = gpio_check_output_vpin(vpin);

    if (ret != HC_HAL_OK) {
        return ret;
    }

    HAL_GPIO_TogglePin(s_gpio_map[vpin].port, s_gpio_map[vpin].pin);
    return HC_HAL_OK;
}

HC_S32 HC_HAL_GPIO_ReadPin(HC_HAL_GPIO_VPin_e vpin, HC_HAL_GPIO_PinState_e *p_state)
{
    HC_S32 ret;

    if (p_state == HC_NULL_PTR) {
        return HC_HAL_ERR_NULL_PTR;
    }

    ret = gpio_check_vpin(vpin);
    if (ret != HC_HAL_OK) {
        return ret;
    }

    *p_state =
        (HAL_GPIO_ReadPin(s_gpio_map[vpin].port, s_gpio_map[vpin].pin) == GPIO_PIN_SET) ? HC_PIN_SET : HC_PIN_RESET;
    return HC_HAL_OK;
}

HC_VOID HC_HAL_GPIOA_IRQHandler(HC_VOID)
{
    /*
     * 兼容入口：主公共头的统一命名仍是 GPIOA/B。
     * 实际 STM32 项目中请让真实 EXTI 向量转发到 HAL_GPIO_EXTI_IRQHandler()。
     */
    HAL_GPIO_EXTI_IRQHandler(HC_HAL_EXAMPLE_STM32_VPIN_KEY_1_PIN);
    HAL_GPIO_EXTI_IRQHandler(HC_HAL_EXAMPLE_STM32_VPIN_KEY_2_PIN);
}

HC_VOID HC_HAL_GPIOB_IRQHandler(HC_VOID)
{
    HAL_GPIO_EXTI_IRQHandler(HC_HAL_EXAMPLE_STM32_VPIN_EXTI_1_PIN);
}

HC_VOID EXTI9_5_IRQHandler(HC_VOID)
{
    HAL_GPIO_EXTI_IRQHandler(HC_HAL_EXAMPLE_STM32_VPIN_KEY_2_PIN);
}

HC_VOID EXTI15_10_IRQHandler(HC_VOID)
{
    HAL_GPIO_EXTI_IRQHandler(HC_HAL_EXAMPLE_STM32_VPIN_KEY_1_PIN);
    HAL_GPIO_EXTI_IRQHandler(HC_HAL_EXAMPLE_STM32_VPIN_EXTI_1_PIN);
}

HC_VOID HAL_GPIO_EXTI_Callback(HC_U16 gpio_pin)
{
    HC_U8 i;

    for (i = 0u; i < (HC_U8)VPIN_MAX; i++) {
        if ((s_gpio_map[i].use_irq == HC_TRUE) && (s_gpio_map[i].pin == gpio_pin)) {
            HC_HAL_GPIO_Callback((HC_HAL_GPIO_VPin_e)i);
        }
    }
}

HC_WEAK HC_VOID HC_HAL_GPIO_Callback(HC_HAL_GPIO_VPin_e vpin)
{
    HC_UNUSED(vpin);
}
