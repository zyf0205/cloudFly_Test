#include "system.h"
/********************************************************************************	 
 * 系统初始化函数	
 * 包括系统和底层硬件初始化
********************************************************************************/

static bool systemTest(void);


/*底层硬件初始化*/
void systemInit(void)
{
	u8 cnt = 0;
	
	nvicInit();			/*中断配置初始化*/
	extiInit();			/*外部中断初始化*/	
	delay_init(96);		/*delay初始化*/
	ledInit();			/*led初始化*/
	ledseqInit();		/*led灯序列初始化*/
	
	radiolinkInit();/*无线通信初始化*/
	usb_Init();			/*虚拟串口初始化*/
	atkpInit();			/*传输协议初始化*/
	pmInit();			/*电源管理初始化*/
	stabilizerInit();	/*电机 传感器 PID初始化*/
	//expModuleDriverInit();/*扩展模块驱动初始化*/
	
	if(systemTest() == true)
	{	
		while(cnt++ < 5)	/*初始化通过 左上绿灯快闪5次*/
		{
			ledFlashOne(LED_BLUE_L, 50, 50);
			printf("hello world\n");
		}			
	}else
	{		
		while(1)		/*初始化错误 右上红灯间隔1s快闪5次*/
		{
			if(cnt++ > 4)
			{
				cnt=0;
				delay_xms(1000);
			}
			ledFlashOne(LED_RED_R, 50, 50);		
		}
	}

	watchdogInit(WATCHDOG_RESET_MS);	/*看门狗初始化*/
	
}
static bool systemTest(void)
{
	bool pass = true;
	
	pass &= ledTest();
	pass &= pmTest();
	pass &= stabilizerTest();	
	pass &= watchdogTest();
	
	return pass;
}

