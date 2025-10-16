#ifndef __CONFIG_H
#define __CONFIG_H
// #include "nvic.h"
// #include "stdio.h"	/*printf ����*/

/********************************************************************************	 
 * �����ļ�����	
********************************************************************************/

#define BOOTLOADER_SIZE		(16*1024)	/*��������16kb*/
#define CONFIG_PARAM_SIZE	(16*1024) /*���ò����洢��16kb*/

#define CONFIG_PARAM_ADDR 	(FLASH_BASE + BOOTLOADER_SIZE)	/*Config_Params��ʼ��ַ*/
#define FIRMWARE_START_ADDR (FLASH_BASE + BOOTLOADER_SIZE + CONFIG_PARAM_SIZE)	/*�̼���ʼ��ַ*/

/*�ǶȻ����໥ת��ϵ��*/
#define DEG2RAD		0.017453293f	/* ��ת���� ��/180 */
#define RAD2DEG		57.29578f		/* ����ת�� 180/�� */

#define P_NAME "MiniFly"
#define MCU_ID_ADDRESS          0x1FFF7A10 /*id�洢��ַ*/
#define MCU_FLASH_SIZE_ADDRESS  0x1FFF7A22 /*flash������Ϣ�洢��ַ*/


#endif /* __CONFIG_H */
