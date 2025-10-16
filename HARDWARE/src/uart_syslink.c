#include <string.h>
#include "sys.h"
#include "config.h"
#include "uart_syslink.h"
#include "debug_assert.h"
#include "config.h"

/*FreeRtos includes*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

/********************************************************************************
 * usart2 串口通信驱动代码
stm32<->usart2<->NRF51822
********************************************************************************/

#define UARTSLK_DATA_TIMEOUT_MS 1000
#define UARTSLK_DATA_TIMEOUT_TICKS (UARTSLK_DATA_TIMEOUT_MS / portTICK_RATE_MS) /*转换为rtostick数*/
// #define CCR_ENABLE_SET ((u32)0x00000001)																				/**/

static bool isInit = false;

static xSemaphoreHandle waitUntilSendDone;
static xSemaphoreHandle uartBusy;
static xQueueHandle uartslkDataDelivery;

static u8 dmaBuffer[64];
static u8 *outDataIsr;
static u8 dataIndexIsr;
static u8 dataSizeIsr;
static bool isUartDmaInitialized;
static DMA_InitTypeDef DMA_InitStructure;
static u32 initialDMACount;
static u32 remainingDMACount;
static bool dmaIsPaused;

static void uartslkPauseDma(void);
static void uartslkResumeDma(void);

/*串口2 tx端dma初始化*/
void uartslkDmaInit(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

	/* USART TX DMA 通道配置*/
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&UARTSLK_TYPE->DR;			/*dma外设地址 即数据要写入的目标地址*/
	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)dmaBuffer;									/*内存地址:dma缓冲区*/
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;									/*内存地址递增*/
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;							/*内存每次发送一个字节*/
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;					/*内存数据宽度 8位*/
	DMA_InitStructure.DMA_BufferSize = 0;																		/*起始数据传输长度*/
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;				/*外设地址不递增*/
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; /*外设数据宽度 8位*/
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;			/*外设每次接收一个字节*/
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;									/* 内存到外设*/
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;														/*普通模式,不循环*/
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;											/*优先级*/
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;									/*不使用fifo*/
	// DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;		/*FIFO阈值(未使用)*/
	DMA_InitStructure.DMA_Channel = UARTSLK_DMA_CH; /*dma通道*/

	/*dma中断 当dma传输完成时会触发中断*/
	NVIC_InitStructure.NVIC_IRQChannel = UARTSLK_DMA_IRQ;			/*dma发送完成中断*/
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7; /*抢占优先级*/
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;				/*子优先级*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;						/*使能通道*/
	NVIC_Init(&NVIC_InitStructure);														/*初始化*/

	isUartDmaInitialized = true; /*dma初始化成功*/
}

/*串口2初始化*/
void uartslkInit(void)
{
	waitUntilSendDone = xSemaphoreCreateBinary(); /*等待发送完成 二值信号量*/
	uartBusy = xSemaphoreCreateBinary();					/*串口忙 二值信号量*/
	xSemaphoreGive(uartBusy);											/*释放串口忙信号量*/

	uartslkDataDelivery = xQueueCreate(1024, sizeof(u8)); /*队列 1024个消息*/
	// ASSERT(uartslkDataDelivery);

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	/* 使能GPIO 和 UART 时钟*/
	RCC_AHB1PeriphClockCmd(UARTSLK_GPIO_PERIF, ENABLE);
	ENABLE_UARTSLK_RCC(UARTSLK_PERIF, ENABLE);

	GPIO_InitStructure.GPIO_Pin = UARTSLK_GPIO_RX_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; /*复用*/
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; /*上拉输入,数据起始位是低电平*/
	GPIO_Init(UARTSLK_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = UARTSLK_GPIO_TX_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; /*输出速度25MHz*/
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;		/*复用推挽*/
	// GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init(UARTSLK_GPIO_PORT, &GPIO_InitStructure);

	/*端口映射*/
	GPIO_PinAFConfig(UARTSLK_GPIO_PORT, UARTSLK_GPIO_AF_TX_PIN, UARTSLK_GPIO_AF_TX);
	GPIO_PinAFConfig(UARTSLK_GPIO_PORT, UARTSLK_GPIO_AF_RX_PIN, UARTSLK_GPIO_AF_RX);

	USART_InitStructure.USART_BaudRate = 1000000;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(UARTSLK_TYPE, &USART_InitStructure);

	uartslkDmaInit(); /*初始化dma*/

	/*串口2中断配置*/
	NVIC_InitStructure.NVIC_IRQChannel = UARTSLK_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5; /*抢占优先级*/
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/*串口接收数据寄存器非空中断*/
	USART_ITConfig(UARTSLK_TYPE, USART_IT_RXNE, ENABLE);

	/*配置TXEN引脚(NRF流控制) pa0*/
	/*nrf控制pa0电平变化从而实现软件控制dma开关,节省资源*/
	RCC_AHB1PeriphClockCmd(UARTSLK_TXEN_PERIF, ENABLE);

	memset(&GPIO_InitStructure, 0, sizeof(GPIO_InitStructure));
	GPIO_InitStructure.GPIO_Pin = UARTSLK_TXEN_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; /*上拉输入*/
	GPIO_Init(UARTSLK_TXEN_PORT, &GPIO_InitStructure);

	/*PA0外部中断配置*/
	EXTI_InitStructure.EXTI_Line = UARTSLK_TXEN_EXTI; /*外部中断1*/
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; /*上升和下降沿*/
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;											 /*使能*/
	EXTI_Init(&EXTI_InitStructure);
	EXTI_ClearITPendingBit(UARTSLK_TXEN_EXTI); /*清除中断标志位*/

	NVIC_EnableIRQ(EXTI0_IRQn); /*使能中断通道*/

	USART_Cmd(UARTSLK_TYPE, ENABLE); /*开启串口2*/
	isInit = true;									 /*初始化成功*/
}

bool uartslkTest(void)
{
	return isInit;
}

/*从接收队列拿数据(带超时处理)*/
bool uartslkGetDataWithTimout(u8 *c)
{
	/*接收uartslkDataDelivery(1024个容量)消息*/
	if (xQueueReceive(uartslkDataDelivery, c, UARTSLK_DATA_TIMEOUT_TICKS) == pdTRUE)
	{
		return true;
	}
	*c = 0; /*超时清零返回的值*/
	return false;
}

/*不使用dma发送原始数据 只能发送八位的数据,会屏蔽高位数据*/
void uartslkSendData(u32 size, u8 *data)
{
	u32 i;

	if (!isInit) /*如果没有初始化直接返回*/
		return;

	for (i = 0; i < size; i++)
	{
#ifdef UARTSLK_SPINLOOP_FLOWCTRL
		while (GPIO_ReadInputDataBit(UARTSLK_TXEN_PORT, UARTSLK_TXEN_PIN) == Bit_SET)
			;
#endif
		/*等待直到发送寄存器为空*/
		while (!(UARTSLK_TYPE->SR & USART_FLAG_TXE))
		{
		};
		UARTSLK_TYPE->DR = (data[i] & 0x00FF); /*屏蔽高位数据*/
	}
}

/*中断方式发送原始数据*/
void uartslkSendDataIsrBlocking(u32 size, u8 *data)
{
	xSemaphoreTake(uartBusy, portMAX_DELAY); /*获取信号量*/
	outDataIsr = data;
	dataSizeIsr = size;
	dataIndexIsr = 1;
	uartslkSendData(1, &data[0]);
	USART_ITConfig(UARTSLK_TYPE, USART_IT_TXE, ENABLE); /*使能串口发送数据寄存器为空中断*/
	/*
	这里进入中断
	*/
	xSemaphoreTake(waitUntilSendDone, portMAX_DELAY);		/*等待发送完成信号量*/
	outDataIsr = 0;
	xSemaphoreGive(uartBusy);
}

/*发送一个字符到串口*/
int uartslkPutchar(int ch)
{
	uartslkSendData(1, (u8 *)&ch);

	return (u8)ch;
}

/*通过DMA发送原始数据*/
void uartslkSendDataDmaBlocking(u32 size, u8 *data)
{
	if (isUartDmaInitialized)
	{
		xSemaphoreTake(uartBusy, portMAX_DELAY);
		while (DMA_GetCmdStatus(UARTSLK_DMA_STREAM) != DISABLE)
			;																									 /*等待DMA空闲*/
		memcpy(dmaBuffer, data, size);											 /*复制数据到DMA缓冲区*/
		DMA_InitStructure.DMA_BufferSize = size;						 /*设置传输长度*/
		initialDMACount = size;															 /*初始传输长度*/
		DMA_Init(UARTSLK_DMA_STREAM, &DMA_InitStructure);		 /*重新初始化DMA数据流*/
		DMA_ITConfig(UARTSLK_DMA_STREAM, DMA_IT_TC, ENABLE); /*开启DMA传输完成中断*/
		USART_DMACmd(UARTSLK_TYPE, USART_DMAReq_Tx, ENABLE); /* 使能USART DMA TX请求 */
		USART_ClearFlag(UARTSLK_TYPE, USART_FLAG_TC);				 /* 清除传输完成中断标志位 */
		DMA_Cmd(UARTSLK_DMA_STREAM, ENABLE);								 /* 使能DMA USART TX数据流 */
		xSemaphoreTake(waitUntilSendDone, portMAX_DELAY);
		xSemaphoreGive(uartBusy);
	}
}
/*暂停DMA传输*/
static void uartslkPauseDma()
{
	if (DMA_GetCmdStatus(UARTSLK_DMA_STREAM) == ENABLE)
	{
		DMA_ITConfig(UARTSLK_DMA_STREAM, DMA_IT_TC, DISABLE); /*关闭DMA传输完成中断*/
		DMA_Cmd(UARTSLK_DMA_STREAM, DISABLE);
		while (DMA_GetCmdStatus(UARTSLK_DMA_STREAM) != DISABLE)
			;
		DMA_ClearITPendingBit(UARTSLK_DMA_STREAM, UARTSLK_DMA_IT_TCIF);
		remainingDMACount = DMA_GetCurrDataCounter(UARTSLK_DMA_STREAM);
		dmaIsPaused = true;
	}
}
/*恢复DMA传输*/
static void uartslkResumeDma()
{
	if (dmaIsPaused)
	{
		DMA_SetCurrDataCounter(UARTSLK_DMA_STREAM, remainingDMACount);									 /*更新DMA计数器*/
		UARTSLK_DMA_STREAM->M0AR = (u32)&dmaBuffer[initialDMACount - remainingDMACount]; /*更新内存读取地址*/
		DMA_ITConfig(UARTSLK_DMA_STREAM, DMA_IT_TC, ENABLE);														 /*开启DMA传输完成中断*/
		USART_ClearFlag(UARTSLK_TYPE, USART_FLAG_TC);																		 /* 清除传输完成中断标志位 */
		DMA_Cmd(UARTSLK_DMA_STREAM, ENABLE);																						 /* 使能DMA USART TX数据流 */
		dmaIsPaused = false;
	}
}

/*dma传输完成中断服务函数*/
void uartslkDmaIsr(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	DMA_ITConfig(UARTSLK_DMA_STREAM, DMA_IT_TC, DISABLE);
	DMA_ClearITPendingBit(UARTSLK_DMA_STREAM, UARTSLK_DMA_IT_TCIF);
	USART_DMACmd(UARTSLK_TYPE, USART_DMAReq_Tx, DISABLE);
	DMA_Cmd(UARTSLK_DMA_STREAM, DISABLE);

	remainingDMACount = 0;
	/*释放二值信号量 设置标志,在中断结束后如果有高优先级在等待这个信号量就切换*/
	xSemaphoreGiveFromISR(waitUntilSendDone, &xHigherPriorityTaskWoken);
}

// 串口中断服务函数
void uartslkIsr(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;/*用于上下文切换*/

	if ((UARTSLK_TYPE->SR & (1 << 5)) != 0) /*接收非空中断*/
	{
		u8 rxDataInterrupt = (u8)(UARTSLK_TYPE->DR & 0xFF); /*得到低八位*/
		xQueueSendFromISR(uartslkDataDelivery, &rxDataInterrupt, &xHigherPriorityTaskWoken);/*添加到队列中,同时检查是否有高优先级任务等待,有的话唤醒*/
	}
	else if (USART_GetITStatus(UARTSLK_TYPE, USART_IT_TXE) == SET)/*发送寄存器为空中断*/
	{
		if (outDataIsr && (dataIndexIsr < dataSizeIsr))/*还有数据没有发送完*/
		{
			USART_SendData(UARTSLK_TYPE, outDataIsr[dataIndexIsr] & 0x00FF);
			dataIndexIsr++;/*索引自增*/
		}
		else/*数据发送完成*/
		{
			USART_ITConfig(UARTSLK_TYPE, USART_IT_TXE, DISABLE);/*关闭发送寄存器为空中断*/
			xSemaphoreGiveFromISR(waitUntilSendDone, &xHigherPriorityTaskWoken);/*释放信号量,并设置标志唤醒*/
		}
	}
}

void uartslkTxenFlowctrlIsr()
{
	EXTI_ClearFlag(UARTSLK_TXEN_EXTI);
	if (GPIO_ReadInputDataBit(UARTSLK_TXEN_PORT, UARTSLK_TXEN_PIN) == Bit_SET)
	{
		uartslkPauseDma();
		// ledSet(LED_GREEN_R, 1);
	}
	else
	{
		uartslkResumeDma();
		// ledSet(LED_GREEN_R, 0);
	}
}

void __attribute__((used)) EXTI0_Callback(void)
{
	uartslkTxenFlowctrlIsr();
}

void __attribute__((used)) USART2_IRQHandler(void)
{
	uartslkIsr();
}

void __attribute__((used)) DMA1_Stream6_IRQHandler(void)
{
	uartslkDmaIsr();
}
