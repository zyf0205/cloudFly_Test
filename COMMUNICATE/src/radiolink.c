#include <string.h>
#include "config.h"
#include "radiolink.h"
#include "config_param.h"
#include "led.h"
#include "ledseq.h"
#include "uart_syslink.h"

/*FreeRtos includes*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

/********************************************************************************
 * 无线通信驱动代码	飞机和遥控器 接收和回传
 ********************************************************************************/

#define RADIOLINK_TX_QUEUE_SIZE 30 /*发送队列个数*/

/*接收状态枚举*/
static enum {
	waitForStartByte1,
	waitForStartByte2,
	waitForMsgID,
	waitForDataLength,
	waitForData,
	waitForChksum1,
} rxState;
static bool isInit;
static atkp_t txPacket;			 /*要发送的数据包 飞机->遥控器*/
static atkp_t rxPacket;			 /*要接受的数据包 遥控器->飞机*/
static xQueueHandle txQueue; /*发送队列句柄*/

static void atkpPacketDispatch(atkp_t *rxPacket);

// radiolink接收ATKPPacket任务
void radiolinkTask(void *param)
{
	rxState = waitForStartByte1;/*接收状态*/

	u8 c;
	u8 dataIndex = 0;
	u8 cksum = 0;

	while (1)
	{
		if (uartslkGetDataWithTimout(&c))/*从接收队列拿数据,阻塞1s*/
		{
			switch (rxState)
			{
			case waitForStartByte1:
				rxState = (c == DOWN_BYTE1) ? waitForStartByte2 : waitForStartByte1;
				cksum = c;
				break;
			case waitForStartByte2:
				rxState = (c == DOWN_BYTE2) ? waitForMsgID : waitForStartByte1;
				cksum += c;
				break;
			case waitForMsgID:
				rxPacket.msgID = c;/*存储id*/
				rxState = waitForDataLength;
				cksum += c;
				break;
			case waitForDataLength:
				if (c <= ATKP_MAX_DATA_SIZE)/*检查数据长度*/
				{
					rxPacket.dataLen = c;/*设置数据包长度*/
					dataIndex = 0;/*数据索引为0*/
					rxState = (c > 0) ? waitForData : waitForChksum1; /*c=0,数据长度为0，校验1*/
					cksum += c;
				}
				else/*数据长度非法*/
				{
					rxState = waitForStartByte1;
				}
				break;
			case waitForData:
				rxPacket.data[dataIndex] = c;
				dataIndex++;
				cksum += c;
				if (dataIndex == rxPacket.dataLen)
				{
					rxState = waitForChksum1;
				}
				break;
			case waitForChksum1:
				if (cksum == c) /*所有校验正确*/
				{
					atkpPacketDispatch(&rxPacket);
				}
				// else /*校验错误*/
				// {
				// 	rxState = waitForStartByte1;
				// 	IF_DEBUG_ASSERT(1);
				// }
				rxState = waitForStartByte1;/*无论校验正确与否都要设置*/
				break;
			default:
				ASSERT(0);
				break;
			}
		}
		else /*超时处理*/
		{
			rxState = waitForStartByte1;
		}
	}
}

void radiolinkInit(void)
{
	if (isInit)
		return;
	uartslkInit();

	/*创建发送队列，CRTP_TX_QUEUE_SIZE个消息*/
	txQueue = xQueueCreate(RADIOLINK_TX_QUEUE_SIZE, sizeof(atkp_t));
	ASSERT(txQueue);

	isInit = true;
}

/*打包ATKPPacket数据通过串口DMA发送*/
static void uartSendPacket(atkp_t *p)
{
	int dataSize;
	u8 cksum = 0;
	u8 sendBuffer[36];

	ASSERT(p->dataLen <= ATKP_MAX_DATA_SIZE);

	sendBuffer[0] = UP_BYTE1;
	sendBuffer[1] = UP_BYTE2;
	sendBuffer[2] = p->msgID;
	sendBuffer[3] = p->dataLen;

	memcpy(&sendBuffer[4], p->data, p->dataLen);
	dataSize = p->dataLen + 5; // 加上cksum
	/*计算校验和*/
	for (int i = 0; i < dataSize - 1; i++)
	{
		cksum += sendBuffer[i];
	}
	sendBuffer[dataSize - 1] = cksum;

	/*串口DMA发送*/
	uartslkSendDataDmaBlocking(dataSize, sendBuffer);
}

/*radiolink接收到ATKPPacket预处理*/
static void atkpPacketDispatch(atkp_t *rxPacket)
{
	atkpReceivePacketBlocking(rxPacket);

	if (rxPacket->msgID == DOWN_POWER)
	{
		;
	} /*do noting*/
	else
	{
		ledseqRun(DATA_RX_LED, seq_linkup);
		/*接收到一个遥控无线数据包则发送一个包*/
		if (xQueueReceive(txQueue, &txPacket, 0) == pdTRUE)
		{
			ASSERT(txPacket.dataLen <= ATKP_MAX_DATA_SIZE);
			ledseqRun(DATA_TX_LED, seq_linkup);
			uartSendPacket(&txPacket);
		}
	}
}

bool radiolinkSendPacket(const atkp_t *p)
{
	ASSERT(p);
	ASSERT(p->dataLen <= ATKP_MAX_DATA_SIZE);
	return xQueueSend(txQueue, p, 0);
}

bool radiolinkSendPacketBlocking(const atkp_t *p)
{
	ASSERT(p);
	ASSERT(p->dataLen <= ATKP_MAX_DATA_SIZE);
	return xQueueSend(txQueue, p, portMAX_DELAY);
}

// 获取剩余可用txQueue个数
int radiolinkGetFreeTxQueuePackets(void)
{
	return (RADIOLINK_TX_QUEUE_SIZE - uxQueueMessagesWaiting(txQueue));
}
