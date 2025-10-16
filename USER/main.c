#include "system.h" /*头文件集合*/

TaskHandle_t startTaskHandle;     /*开始任务句柄*/
static void startTask(void *arg); /*开始任务函数*/

int main()
{
    systemInit(); /*底层硬件初始化*/

    xTaskCreate(startTask, "START_TASK", 300, NULL, 2, &startTaskHandle); /*创建起始任务*/

    vTaskStartScheduler(); /*开启任务调度*/

    while (1) {};
}

/*创建任务*/
void startTask(void *arg)
{
    taskENTER_CRITICAL(); /*进入临界区*/

    xTaskCreate(radiolinkTask, "RADIOLINK", 150, NULL, 5, NULL); /*创建无线连接任务*/

    xTaskCreate(atkpTxTask, "ATKP_TX", 150, NULL, 3, NULL);        /*将传感器数据进行打包,添加到待发送队列*/
    xTaskCreate(atkpRxAnlTask, "ATKP_RX_ANL", 300, NULL, 6, NULL); /*将接收到的数据包进行解析*/

    xTaskCreate(pmTask, "PWRMGNT", 150, NULL, 2, NULL); /*创建电源管理任务*/

    xTaskCreate(sensorsTask, "SENSORS", 450, NULL, 4, NULL); /*创建传感器处理任务*/

    xTaskCreate(stabilizerTask, "STABILIZER", 450, NULL, 5, NULL); /*创建姿态任务*/

    // printf("Free heap: %d bytes\n", xPortGetFreeHeapSize()); /*打印剩余堆栈大小*/

    vTaskDelete(startTaskHandle); /*删除开始任务*/

    taskEXIT_CRITICAL(); /*退出临界区*/
}

void vApplicationIdleHook(void)
{
    static u32 tickWatchdogReset = 0;

    portTickType tickCount = getSysTickCnt();

    if (tickCount - tickWatchdogReset > WATCHDOG_RESET_MS) {
        tickWatchdogReset = tickCount;
        watchdogReset(); /*喂狗*/
    }

    __WFI(); /*进入低功耗模式*/
}
