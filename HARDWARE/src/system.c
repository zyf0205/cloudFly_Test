#include "system.h"
/********************************************************************************	 
 * ϵͳ��ʼ������	
 * ����ϵͳ�͵ײ�Ӳ����ʼ��
********************************************************************************/

static bool systemTest(void);


/*�ײ�Ӳ����ʼ��*/
void systemInit(void)
{
	u8 cnt = 0;
	
	nvicInit();			/*�ж����ó�ʼ��*/
	extiInit();			/*�ⲿ�жϳ�ʼ��*/	
	delay_init(96);		/*delay��ʼ��*/
	ledInit();			/*led��ʼ��*/
	ledseqInit();		/*led�����г�ʼ��*/
	
	radiolinkInit();/*����ͨ�ų�ʼ��*/
	usb_Init();			/*���⴮�ڳ�ʼ��*/
	atkpInit();			/*����Э���ʼ��*/
	pmInit();			/*��Դ�����ʼ��*/
	stabilizerInit();	/*��� ������ PID��ʼ��*/
	//expModuleDriverInit();/*��չģ��������ʼ��*/
	
	if(systemTest() == true)
	{	
		while(cnt++ < 5)	/*��ʼ��ͨ�� �����̵ƿ���5��*/
		{
			ledFlashOne(LED_BLUE_L, 50, 50);
			printf("hello world\n");
		}			
	}else
	{		
		while(1)		/*��ʼ������ ���Ϻ�Ƽ��1s����5��*/
		{
			if(cnt++ > 4)
			{
				cnt=0;
				delay_xms(1000);
			}
			ledFlashOne(LED_RED_R, 50, 50);		
		}
	}

	watchdogInit(WATCHDOG_RESET_MS);	/*���Ź���ʼ��*/
	
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

