#include "config.h"
#include "watchdog.h"
#include "debug_assert.h"

/********************************************************************************	 
 * ���Ź���������	
********************************************************************************/


bool watchdogTest(void)
{
	bool wasNormalStart = true;

	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST)) 
	{
		RCC_ClearFlag();
		wasNormalStart = false;
		printf("The system resumed after watchdog timeout [WARNING]\n");
		printAssertSnapshotData();
	}
	return wasNormalStart;
}


void watchdogInit(u16 xms)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);// ���д����
	IWDG_SetPrescaler(IWDG_Prescaler_32);/*���÷�Ƶ��,ʹ��LSIʱ��Դ47000Hz*/

	/* 47000/32Hz => 1.47KHz  1ms*/
	IWDG_SetReload((u16)(1.47*xms));/*��������ֵ(ms*ÿmsִ�м������)*/
	/*��ʱʱ��=����ֵ / IWDGʱ��Ƶ��=(1.47 �� xms) / 1470Hz*/

	watchdogReset();/*���ÿ��Ź�*/
	IWDG_Enable();/*�������Ź�*/
}
