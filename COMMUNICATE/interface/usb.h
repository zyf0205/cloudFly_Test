#ifndef __USB_H__
#define __USB_H__

#include "usbd_cdc_vcp.h"
#include <stdio.h>

void usbTask(void *arg);
void usb_Init(void);
void usb_receive(void);

#endif
