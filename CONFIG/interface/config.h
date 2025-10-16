#ifndef __CONFIG_H
#define __CONFIG_H
// #include "nvic.h"
// #include "stdio.h"	/*printf 调用*/

/********************************************************************************	 
 * 配置文件代码	
********************************************************************************/

#define BOOTLOADER_SIZE		(16*1024)	/*引导程序16kb*/
#define CONFIG_PARAM_SIZE	(16*1024) /*配置参数存储区16kb*/

#define CONFIG_PARAM_ADDR 	(FLASH_BASE + BOOTLOADER_SIZE)	/*Config_Params起始地址*/
#define FIRMWARE_START_ADDR (FLASH_BASE + BOOTLOADER_SIZE + CONFIG_PARAM_SIZE)	/*固件起始地址*/

/*角度弧度相互转换系数*/
#define DEG2RAD		0.017453293f	/* 度转弧度 π/180 */
#define RAD2DEG		57.29578f		/* 弧度转度 180/π */

#define P_NAME "MiniFly"
#define MCU_ID_ADDRESS          0x1FFF7A10 /*id存储地址*/
#define MCU_FLASH_SIZE_ADDRESS  0x1FFF7A22 /*flash容量信息存储地址*/


#endif /* __CONFIG_H */
