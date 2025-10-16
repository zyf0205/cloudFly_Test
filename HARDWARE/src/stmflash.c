#include "stmflash.h"
#include "usb.h"

/********************************************************************************
 * STM32�ڲ�FLASH��д ��������
 ********************************************************************************/

// ��ȡָ����ַ�İ���(16λ����)
// faddr:����ַ
// ����ֵ:��Ӧ����.
u32 STMFLASH_ReadWord(u32 faddr)
{
	return *(vu32 *)faddr; /*ǿ��תΪvolatile unsigned 32-bitָ��,�ٽ�ָ��õ�����*/
}

// ��ȡĳ����ַ���ڵ�flash����
// addr:flash��ַ
// ����ֵ:0~11,��addr���ڵ�����
uint16_t STMFLASH_GetFlashSector(u32 addr)
{
	if (addr < ADDR_FLASH_SECTOR_1)
		return FLASH_Sector_0;
	else if (addr < ADDR_FLASH_SECTOR_2)
		return FLASH_Sector_1;
	else if (addr < ADDR_FLASH_SECTOR_3)
		return FLASH_Sector_2;
	else if (addr < ADDR_FLASH_SECTOR_4)
		return FLASH_Sector_3;
	else if (addr < ADDR_FLASH_SECTOR_5)
		return FLASH_Sector_4;
	else if (addr < ADDR_FLASH_SECTOR_6)
		return FLASH_Sector_5;
	else if (addr < ADDR_FLASH_SECTOR_7)
		return FLASH_Sector_6;
	else if (addr < ADDR_FLASH_SECTOR_8)
		return FLASH_Sector_7;
	else if (addr < ADDR_FLASH_SECTOR_9)
		return FLASH_Sector_8;
	else if (addr < ADDR_FLASH_SECTOR_10)
		return FLASH_Sector_9;
	else if (addr < ADDR_FLASH_SECTOR_11)
		return FLASH_Sector_10;
	return FLASH_Sector_11;
}

// ��ָ����ַ��ʼд��ָ�����ȵ�����
// �ر�ע��:��ΪSTM32F4������ʵ��̫��,û�취���ر�����������,���Ա�����
//          д��ַ�����0XFF,��ô���Ȳ������������Ҳ�������������.����
//          д��0XFF�ĵ�ַ,�����������������ݶ�ʧ.����д֮ǰȷ��������
//          û����Ҫ����,��������������Ȳ�����,Ȼ����������д.
// �ú�����OTP����Ҳ��Ч!��������дOTP��!
// OTP�����ַ��Χ:0X1FFF7800~0X1FFF7A0F
// WriteAddr:��ʼ��ַ(�˵�ַ����Ϊ4�ı���!!)
// pBuffer:����ָ��
// NumToWrite:��(32λ)��(����Ҫд���32λ���ݵĸ���.)
void STMFLASH_Write(u32 WriteAddr, u32 *pBuffer, u32 NumToWrite)
{
	FLASH_Status status = FLASH_COMPLETE; /*�����ɹ�ö�ٱ���*/
	u32 addrx = 0;
	u32 endaddr = 0;
	if (WriteAddr < STM32_FLASH_BASE || WriteAddr % 4)
		return;										 // �Ƿ���ַ
	FLASH_Unlock();							 // ����
	FLASH_DataCacheCmd(DISABLE); // FLASH�����ڼ�,�����ֹ���ݻ���,��ֹ�ӻ����ж�ȡ����ֵ

	addrx = WriteAddr;										// д�����ʼ��ַ
	endaddr = WriteAddr + NumToWrite * 4; // д��Ľ�����ַ
	if (addrx < 0X1FFF0000)								// ֻ�����洢��,����Ҫִ�в�������!!STM32F4 ���� Flash ����ַ��Χ�� 0x08000000 ~ 0x1FFF0000
	{
		while (addrx < endaddr) // ɨ��һ���ϰ�.(�Է�FFFFFFFF�ĵط�,�Ȳ���)
		{
			if (STMFLASH_ReadWord(addrx) != 0XFFFFFFFF) // �з�0XFFFFFFFF�ĵط�,Ҫ������������
			{
				status = FLASH_EraseSector(STMFLASH_GetFlashSector(addrx), VoltageRange_3); // VCC=2.7~3.6V֮��!!
				if (status != FLASH_COMPLETE)
					break; // ����������
			}
			else
				addrx += 4;
		}
	}
	if (status == FLASH_COMPLETE)
	{
		while (WriteAddr < endaddr) // д����
		{
			if (FLASH_ProgramWord(WriteAddr, *pBuffer) != FLASH_COMPLETE) // д������
			{
				break; // д���쳣
			}
			WriteAddr += 4;
			pBuffer++; /*ָ���һ*/
		}
	}
	FLASH_DataCacheCmd(ENABLE); // FLASH��������,�������ݻ���
	FLASH_Lock();								// ����
}

// ��ָ����ַ��ʼ����ָ�����ȵ�����
// ReadAddr:��ʼ��ַ
// pBuffer:����ָ��,ָ��洢��ȡ�����ݵ�������׵�ַ
// NumToRead:��(4λ)��
void STMFLASH_Read(u32 ReadAddr, u32 *pBuffer, u32 NumToRead)
{
	u32 i;
	for (i = 0; i < NumToRead; i++)
	{
		pBuffer[i] = STMFLASH_ReadWord(ReadAddr); // ��ȡ4���ֽ�.
		ReadAddr += 4;														// ƫ��4���ֽ�.
	}
}

/*ʹ�����⴮�ڴ�ӡotp����*/
void Read_OTP(void)
{
	uint32_t *otp_ptr = (uint32_t *)0x1FFF7800;
	for (int i = 0; i < 16; i++) // 16 ��
	{
		printf("OTP Row %02d: ", i);
		for (int j = 0; j < 8; j++) // ÿ�� 8 ���֣�32 �ֽڣ�
		{
			printf("%08X ", otp_ptr[i * 8 + j]);
		}
		printf("\r\n");
	}
}
