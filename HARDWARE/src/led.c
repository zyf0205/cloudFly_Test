#include "led.h"

/********************************************************************************
 * LED驱动代码
********************************************************************************/

/*LED 极性*/
#define LED_POL_POS 0
#define LED_POL_NEG 1

static bool isInit = false;

typedef struct
{
    GPIO_TypeDef* port;
    u16 pin;
    int polarity;
} led_t;

static led_t leds[LED_NUM] =
{
    [LED_BLUE_L]	= {GPIOB, GPIO_Pin_12, LED_POL_POS},
    [LED_GREEN_L]	= {GPIOA, GPIO_Pin_6,  LED_POL_NEG},
    [LED_RED_L] 	= {GPIOA, GPIO_Pin_7,  LED_POL_NEG},
    [LED_GREEN_R]	= {GPIOC, GPIO_Pin_13, LED_POL_NEG},
    [LED_RED_R] 	= {GPIOC, GPIO_Pin_14, LED_POL_NEG},
};

/* 
LED初始化 
红/绿      红/绿

蓝         未使用
*/
void ledInit(void)/*初始化gpio并关闭所有led*/
{
    if(isInit)	return;

    GPIO_InitTypeDef GPIO_InitStructure;

    /*使能led时钟*/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    /*LED_GREEN_L PA6	LED_RED_L PA7  左上双色灯*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;/*推挽输出*/
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*LED_BLUE_L PB12  左蓝色灯*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /*LED_GREEN_R PC13	LED_RED_R PC14  右上双色灯*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    ledClearAll();

    isInit = true;

}

/* LED测试 */
bool ledTest(void)
{
    ledSet(LED_GREEN_L, 1);
    ledSet(LED_GREEN_R, 1);
    ledSet(LED_RED_L, 0);
    ledSet(LED_RED_R, 0);
    delay_xms(250);

    ledSet(LED_GREEN_L, 0);
    ledSet(LED_GREEN_R, 0);
    ledSet(LED_RED_L, 1);
    ledSet(LED_RED_R, 1);

    delay_xms(250);

    ledClearAll();
    ledSet(LED_BLUE_L, 1);

    return isInit;
}

/*关闭所有LED*/
void ledClearAll(void)
{
    for(u8 i = 0; i < LED_NUM; i++)
    {
        ledSet((led_e)i, 0);
    }
}

/*打开所有LED*/
void ledSetAll(void)
{
    for(u8 i = 0; i < LED_NUM; i++)
    {
        ledSet((led_e)i, 1);
    }
}
/*LED闪烁1次*/
void ledFlashOne(led_e led, u32 onTime, u32 offTime)
{
    ledSet(led, 1);
    delay_xms(onTime);/*点亮时间*/
    ledSet(led, 0);
    delay_xms(offTime);/*熄灭时间*/
}

/* 设置某个LED的状态 */
void ledSet(led_e led, bool value)
{
    if (led > LED_NUM)/*超过枚举值,返回*/
        return;

    if (leds[led].polarity == LED_POL_NEG)/*检测led极性*/
        value = !value;/*根据极性改变value*/

    if(value)
        GPIO_SetBits(leds[led].port, leds[led].pin);
    else
        GPIO_ResetBits(leds[led].port, leds[led].pin);
}


