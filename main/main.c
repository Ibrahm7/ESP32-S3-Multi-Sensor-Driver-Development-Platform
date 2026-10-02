#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_log.h"

#define BOOT_PIN GPIO_NUM_0

static const char *TAG = "MAIN";

static QueueHandle_t button_queue;

void IRAM_ATTR button_isr_handler(void* param){
    int64_t timestamp = esp_timer_get_time();
    BaseType_t higher_priority_task = pdFALSE;

    xQueueSendFromISR(button_queue,&timestamp,&higher_priority_task); //transfer the time to button_queue and check is higher_priority_task true

    if(higher_priority_task){
        portYIELD_FROM_ISR(); //if higher_priority_task is true then go to higher priority task
    }
}

void button_event_task(void* arg){
    int64_t receivedVal;

    while(1){
        if(xQueueReceive(button_queue,&receivedVal,portMAX_DELAY) == pdTRUE){
            ESP_LOGI(TAG,"Kesme tetiklendi, zaman: %lld us",receivedVal);
        }
    }
}


void app_main(void)
{
   button_queue = xQueueCreate(5,sizeof(int64_t));

   gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << BOOT_PIN),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_ANYEDGE
   };
   gpio_config(&io_conf);

   gpio_install_isr_service(0);
   gpio_isr_handler_add(BOOT_PIN,button_isr_handler,(void*)BOOT_PIN);


   xTaskCreate(button_event_task,"button_event",4096,NULL,5,NULL);
}