#include "delay.h"
#include "sys.h"

//如果使用OS,则包括下面的头文件（以ucos为例）即可.
#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"					//FreeRTOS使用	 
#include "task.h"	  
#endif


static u32 fac_us=0;							//us延时倍乘数

#if SYSTEM_SUPPORT_OS		
    static u16 fac_ms=0;				        //ms延时倍乘数,在os下,代表每个节拍的ms数
#endif

			   
//初始化延迟函数
//当使用OS的时候,此函数会初始化OS的时钟节拍
//SYSTICK的时钟固定为AHB时钟的1/8
//SYSCLK:系统时钟频率
void delay_init(u8 SYSCLK)
{
#if SYSTEM_SUPPORT_OS 						//如果需要支持OS.
	u32 reload;
#endif
 	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK); /*使用系统时钟hclk作为时钟源*/
	fac_us=SYSCLK;							/*保存系统频率,用于us延时计算*/
#if SYSTEM_SUPPORT_OS 						
	reload=SYSCLK;							//每秒钟的计数次数 单位为M	   
	reload*=1000000/configTICK_RATE_HZ;		//计算一个freertos时钟节拍需要多少个时钟计数(周期)96000次

	fac_ms=1000/configTICK_RATE_HZ;			//1s(1000ms)/每秒次数(频率)->每个rtos周期相当于多少个ms,这里等于1ms  
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;//开启SYSTICK中断
	SysTick->LOAD=reload; 					//cpu计数1次:1/96000000s,则中断间隔=1/96000000*96000=1/1000s=1ms则1ms进一次systick中断	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;	//启动计数器 
#else
	fac_ms=(u16)fac_us*1000;				//非OS下,代表每个ms需要的systick时钟数   
#endif
}								    

#if SYSTEM_SUPPORT_OS 						//如果需要支持OS.
//延时nus
//nus:要延时的us数.	
//nus:0~44739242(最大值即2^32/fac_us@fac_us=96)	    								   
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;				//LOAD的值	    	 
	ticks=nus*fac_us; 						//需要的节拍数 
	told=SysTick->VAL;        				//刚进入时的计数器值
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;	//这里注意一下SYSTICK是一个递减的计数器就可以了.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;			//时间超过/等于要延迟的时间,则退出.
		}  
	};											    
} 
//延时nms
//nms:要延时的ms数
//nms:0~65535
void delay_ms(u16 nms)
{	
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)//系统已经运行	    
	{		 
		if(nms>=fac_ms)						//延时的时间大于OS的最少时间周期 
		{ 
   			vTaskDelay(nms/fac_ms);	 		//FreeRTOS延时
		}
		nms%=fac_ms;						//OS已经无法提供这么小的延时了,采用普通方式延时    
	}
	delay_us((u32)(nms*1000));				//普通方式延时
}
#else  //不用ucos时
//延时nus
//nus为要延时的us数.	
//注意:nus的值,不要大于174762us(最大值即2^24/fac_us@fac_us=96)
void delay_us(u32 nus)
{		
	u32 temp;	    	 
	SysTick->LOAD=nus*fac_us; 				//时间加载	  		 
	SysTick->VAL=0x00;        				//清空计数器
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ; //开始倒数 	 
	do
	{
		temp=SysTick->CTRL;
	}while((temp&0x01)&&!(temp&(1<<16)));	//等待时间到达   
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk; //关闭计数器
	SysTick->VAL =0X00;       				//清空计数器 
}
//延时nms
//注意nms的范围
//SysTick->LOAD为24位寄存器,所以,最大延时为:
//nms<=0xffffff*8*1000/SYSCLK
//SYSCLK单位为Hz,nms单位为ms
//对168M条件下,nms<=798ms 
//void delay_xms(u16 nms)
//{	 		  	  
//	u32 temp;		   
//	SysTick->LOAD=(u32)nms*fac_ms;			//时间加载(SysTick->LOAD为24bit)
//	SysTick->VAL =0x00;           			//清空计数器
//	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //开始倒数 
//	do
//	{
//		temp=SysTick->CTRL;
//	}while((temp&0x01)&&!(temp&(1<<16)));	//等待时间到达   
//	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //关闭计数器
//	SysTick->VAL =0X00;     		  		//清空计数器	  	    
//} 
//延时nms 
//nms:0~65535
void delay_ms(u16 nms)
{	 	 
	u8 repeat=nms/540;						//这里用540,是考虑到某些客户可能超频使用,
											//比如超频到248M的时候,delay_xms最大只能延时541ms左右了
	u16 remain=nms%540;
	while(repeat)
	{
		delay_xms(540);
		repeat--;
	}
	if(remain)delay_xms(remain);
} 
#endif

//延时nms,不会引起任务调度
//nms:要延时的ms数
void delay_xms(u32 nms)
{
	u32 i;
	for(i=0;i<nms;i++) delay_us(1000);
}			 












