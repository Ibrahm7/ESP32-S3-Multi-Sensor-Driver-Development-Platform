#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ad8232_driver.h"
#include "freertos/queue.h"

static const char *TAG = "MAIN";

QueueHandle_t myQueue;
void fake_adc_producer_task(void* pvParameter){
    const TickType_t period = pdMS_TO_TICKS(100);
    TickType_t last_wake_time = xTaskGetTickCount();
    
    static int counter = 0;


    while(1){
        int adc = counter++;
        if(xQueueSend(myQueue,&adc,0) != pdTRUE){
            ESP_LOGW(TAG,"Kayıp oldu, değer: %d",adc);
        }
        vTaskDelayUntil(&last_wake_time,period);

    }
}

void fake_adc_consumer_task(void* pvParameter){
    int received_value;
    
    while(1){
        if(xQueueReceive(myQueue,&received_value,portMAX_DELAY) == pdTRUE){
            ESP_LOGI(TAG,"Alınan değer: %d",received_value);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void app_main(void)
{
    myQueue = xQueueCreate(3,sizeof(int));

    xTaskCreate(fake_adc_producer_task,"producer",4096,NULL,5,NULL);
    xTaskCreate(fake_adc_consumer_task,"consumer",4096,NULL,5,NULL);
}