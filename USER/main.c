#include "system.h" /*ͷ�ļ�����*/

TaskHandle_t startTaskHandle;     /*��ʼ������*/
static void startTask(void *arg); /*��ʼ������*/

int main()
{
    systemInit(); /*�ײ�Ӳ����ʼ��*/

    xTaskCreate(startTask, "START_TASK", 300, NULL, 2, &startTaskHandle); /*������ʼ����*/

    vTaskStartScheduler(); /*�����������*/

    while (1) {};
}

/*��������*/
void startTask(void *arg)
{
    taskENTER_CRITICAL(); /*�����ٽ���*/

    xTaskCreate(radiolinkTask, "RADIOLINK", 150, NULL, 5, NULL); /*����������������*/

    xTaskCreate(atkpTxTask, "ATKP_TX", 150, NULL, 3, NULL);        /*�����������ݽ��д��,��ӵ������Ͷ���*/
    xTaskCreate(atkpRxAnlTask, "ATKP_RX_ANL", 300, NULL, 6, NULL); /*�����յ������ݰ����н���*/

    xTaskCreate(pmTask, "PWRMGNT", 150, NULL, 2, NULL); /*������Դ��������*/

    xTaskCreate(sensorsTask, "SENSORS", 450, NULL, 4, NULL); /*������������������*/

    xTaskCreate(stabilizerTask, "STABILIZER", 450, NULL, 5, NULL); /*������̬����*/

    // printf("Free heap: %d bytes\n", xPortGetFreeHeapSize()); /*��ӡʣ���ջ��С*/

    vTaskDelete(startTaskHandle); /*ɾ����ʼ����*/

    taskEXIT_CRITICAL(); /*�˳��ٽ���*/
}

void vApplicationIdleHook(void)
{
    static u32 tickWatchdogReset = 0;

    portTickType tickCount = getSysTickCnt();

    if (tickCount - tickWatchdogReset > WATCHDOG_RESET_MS) {
        tickWatchdogReset = tickCount;
        watchdogReset(); /*ι��*/
    }

    __WFI(); /*����͹���ģʽ*/
}
