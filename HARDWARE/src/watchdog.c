#include "config.h"
#include "watchdog.h"
#include "debug_assert.h"

/********************************************************************************	 
 * 看门狗驱动代码	
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
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);// 解除写保护
	IWDG_SetPrescaler(IWDG_Prescaler_32);/*设置分频器,使用LSI时钟源47000Hz*/

	/* 47000/32Hz => 1.47KHz  1ms*/
	IWDG_SetReload((u16)(1.47*xms));/*设置重载值(ms*每ms执行计算次数)*/
	/*超时时间=重载值 / IWDG时钟频率=(1.47 × xms) / 1470Hz*/

	watchdogReset();/*重置看门狗*/
	IWDG_Enable();/*启动看门狗*/
}
