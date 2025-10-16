#include "usb.h"

// 重定向printf
int fputc(int ch, FILE *f)
{
  u8 data = (u8)ch;
  usbsendData(&data, 1);
  return ch;
}

void usb_Init(void)
{
  usbd_cdc_vcp_Init();
}
