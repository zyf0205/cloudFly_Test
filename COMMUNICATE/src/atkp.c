#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "atkp.h"
#include "radiolink.h"
#include "usblink.h"
#include "usbd_usr.h"
#include "stabilizer.h"
#include "motors.h"
#include "commander.h"
#include "flip.h"
#include "pm.h"
#include "pid.h"
#include "attitude_pid.h"
#include "sensors.h"
#include "position_pid.h"
#include "config_param.h"
#include "power_control.h"
#include "remoter_ctrl.h"
#include "optical_flow.h"
#include "state_estimator.h"

/*FreeRTOS相关头文件*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

/********************************************************************************
 * 无线通信驱动代码
 * STM<->NRF<->遥控器
 ********************************************************************************/

// 数据拆分宏定义
#define BYTE0(dwTemp) (*((u8 *)(&dwTemp)))
#define BYTE1(dwTemp) (*((u8 *)(&dwTemp) + 1))
#define BYTE2(dwTemp) (*((u8 *)(&dwTemp) + 2))
#define BYTE3(dwTemp) (*((u8 *)(&dwTemp) + 3))

// 数据返回周期时间（单位ms）
#define PERIOD_STATUS 30
#define PERIOD_POWER 30

#define ATKP_RX_QUEUE_SIZE 10 /*ATKP包接收队列消息个数*/

typedef struct
{
	u16 roll;
	u16 pitch;
	u16 yaw;
	u16 thrust;
} rcControlData_t;

bool isInit = false;
bool flyable = false;						 /*是否允许起飞*/
static xQueueHandle rxQueue;		 /*接收队列句柄*/

static void atkpSendPacket(atkp_t *p)
{
	/*添加到串口发送到遥控器*/
	radiolinkSendPacket(p);

	/*添加到虚拟串口发送到上位机*/
	// if(getusbConnectState())
	// {
	// 	usblinkSendPacket(p);
	// }
}

/***************************发送至遥控器******************************/
static void sendStatus(float roll, float pitch, float yaw, s32 alt, u8 fly_model, u8 armed)
{
	u8 _cnt = 0;
	atkp_t p;
	vs16 _temp;
	vs32 _temp2 = alt;

	p.msgID = UP_STATUS;

	_temp = (int)(roll * 100);
	p.data[_cnt++] = BYTE1(_temp);
	p.data[_cnt++] = BYTE0(_temp);
	_temp = (int)(pitch * 100);
	p.data[_cnt++] = BYTE1(_temp);
	p.data[_cnt++] = BYTE0(_temp);
	_temp = (int)(yaw * 100);
	p.data[_cnt++] = BYTE1(_temp);
	p.data[_cnt++] = BYTE0(_temp);

	p.data[_cnt++] = BYTE3(_temp2);
	p.data[_cnt++] = BYTE2(_temp2);
	p.data[_cnt++] = BYTE1(_temp2);
	p.data[_cnt++] = BYTE0(_temp2);

	p.data[_cnt++] = fly_model;
	p.data[_cnt++] = armed;

	p.dataLen = _cnt;
	atkpSendPacket(&p);
}

static void sendPower(u16 votage, u16 current)
{
	u8 _cnt = 0;
	atkp_t p;

	p.msgID = UP_POWER;

	p.data[_cnt++] = BYTE1(votage);
	p.data[_cnt++] = BYTE0(votage);
	p.data[_cnt++] = BYTE1(current);
	p.data[_cnt++] = BYTE0(current);

	p.dataLen = _cnt;
	atkpSendPacket(&p);
}

/****************************************************************************/
/*数据周期性发送给上位机，每1ms调用一次*/
static void atkpSendPeriod(void)
{
	static u16 count_ms = 1;
	/*状态数据*/
	if (!(count_ms % PERIOD_STATUS))
	{
		attitude_t attitude;
		getAttitudeData(&attitude); /*获取姿态角*/
		int baroData = getBaroData();
		sendStatus(attitude.roll, attitude.pitch, attitude.yaw, baroData, 0, flyable);
	}
	/*电池电压*/
	if (!(count_ms % PERIOD_POWER))
	{
		float bat = pmGetBatteryVoltage();
		sendPower(bat * 100, 500);
	}
	if (++count_ms >= 65535)
		count_ms = 1;
}

/*数据包解析函数*/
static void atkpReceiveAnl(atkp_t *anlPacket)
{
	/*处理系统控制指令*/
	if (anlPacket->msgID == DOWN_COMMAND)
	{
		switch (anlPacket->data[0])
		{
		case D_COMMAND_FLIGHT_LOCK: /*上锁,静止起飞*/
			flyable = false;
			break;

		case D_COMMAND_FLIGHT_ULOCK: /*解锁*/
			flyable = true;
		}
	}
	/*电源管理数据*/
	else if (anlPacket->msgID == DOWN_POWER) /*nrf51822*/
	{
		pmSyslinkUpdate(anlPacket);
	}
	/*遥控器指令及数据处理*/
	else if (anlPacket->msgID == DOWN_REMOTER) /*遥控器*/
	{
		remoterCtrlProcess(anlPacket);
	}
}

void atkpTxTask(void *param)
{
	sendMsgACK();
	while (1)
	{
		atkpSendPeriod();
		vTaskDelay(1);
	}
}

void atkpRxAnlTask(void *param)
{
	atkp_t p;
	while (1)
	{
		xQueueReceive(rxQueue, &p, portMAX_DELAY); /*从接收队列拿到数据包*/
		atkpReceiveAnl(&p);												 /*分析数据包*/
	}
}

/*初始化*/
void atkpInit(void)
{
	if (isInit)
		return;
	rxQueue = xQueueCreate(ATKP_RX_QUEUE_SIZE, sizeof(atkp_t)); /*创建接收队列*/
	isInit = true;
}

/*把数据包添加到接收队列  在接收到数据包后调用*/
bool atkpReceivePacketBlocking(atkp_t *p)
{
	return xQueueSend(rxQueue, p, portMAX_DELAY);
}
