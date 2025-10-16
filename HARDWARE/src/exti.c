#include <stdbool.h>
#include "sys.h"
#include "exti.h"

/********************************************************************************	 
 * 外部中断驱动代码	
********************************************************************************/


static bool isInit;

/* Interruption initialisation */
void extiInit()
{
	static NVIC_InitTypeDef NVIC_InitStructure;

	if (isInit)	return;
	

	RCC_AHB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); /*使能系统配置控制器*/

	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;/*子优先级都是0*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;/*中断通道使能*/

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;/*抢占优先级 数值越小,优先级越高*/
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;/*中断通道*/
	NVIC_Init(&NVIC_InitStructure);/*初始化*/

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_Init(&NVIC_InitStructure);

	isInit = true;
}

bool extiTest(void)
{
	return isInit;
}

/*
编译器在优化时会检查函数是否被调用
如果发现函数没有被显式调用，可能会将其删除以减小代码体积
但中断服务函数是由硬件自动调用的，编译器无法检测到这种调用关系
*/
void __attribute__((used)) EXTI0_IRQHandler(void)/*防止优化*/
{
	EXTI_ClearITPendingBit(EXTI_Line0);
	EXTI0_Callback();
}

void __attribute__((used)) EXTI1_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line1);
	EXTI1_Callback();
}

void __attribute__((used)) EXTI2_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line2);
	EXTI2_Callback();
}

void __attribute__((used)) EXTI3_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line3);
	EXTI3_Callback();
}

void __attribute__((used)) EXTI4_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line4);
	EXTI4_Callback();
}

void __attribute__((used)) EXTI9_5_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line5) == SET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line5);
		EXTI5_Callback();
	}
	if (EXTI_GetITStatus(EXTI_Line6) == SET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line6);
		EXTI6_Callback();
	}
	if (EXTI_GetITStatus(EXTI_Line7) == SET)
	{
		EXTI_ClearITPendingBit(EXTI_Line7);
		EXTI7_Callback();
	}
	if (EXTI_GetITStatus(EXTI_Line8) == SET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line8);
		EXTI8_Callback();
	}
	if (EXTI_GetITStatus(EXTI_Line9) == SET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line9);
		EXTI9_Callback();
	}
}

void __attribute__((used)) EXTI15_10_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line10) == SET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line10);
		EXTI10_Callback();
	}
	if (EXTI_GetITStatus(EXTI_Line11) == SET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line11);
		EXTI11_Callback();
	}
	if (EXTI_GetITStatus(EXTI_Line12) == SET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line12);
		EXTI12_Callback();
	}
	if (EXTI_GetITStatus(EXTI_Line13) == SET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line13);
		EXTI13_Callback();
	}
	if (EXTI_GetITStatus(EXTI_Line14) == SET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line14);
		EXTI14_Callback();
	}
	if (EXTI_GetITStatus(EXTI_Line15) == SET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line15);
		EXTI15_Callback();
	}
}


/*
提供一个默认的函数实现
允许其他地方重新定义同名函数来覆盖默认实现
如果没有其他定义，就使用弱符号的默认实现
*/
void __attribute__((weak)) EXTI0_Callback(void) { }
void __attribute__((weak)) EXTI1_Callback(void) { }
void __attribute__((weak)) EXTI2_Callback(void) { }
void __attribute__((weak)) EXTI3_Callback(void) { }
void __attribute__((weak)) EXTI4_Callback(void) { }
void __attribute__((weak)) EXTI5_Callback(void) { }
void __attribute__((weak)) EXTI6_Callback(void) { }
void __attribute__((weak)) EXTI7_Callback(void) { }
void __attribute__((weak)) EXTI8_Callback(void) { }
void __attribute__((weak)) EXTI9_Callback(void) { }
void __attribute__((weak)) EXTI10_Callback(void) { }
void __attribute__((weak)) EXTI11_Callback(void) { }
void __attribute__((weak)) EXTI12_Callback(void) { }
void __attribute__((weak)) EXTI13_Callback(void) { }
void __attribute__((weak)) EXTI14_Callback(void) { }
void __attribute__((weak)) EXTI15_Callback(void) { }
