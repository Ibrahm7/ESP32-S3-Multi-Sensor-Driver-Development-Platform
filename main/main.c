#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_err.h"

static const char* TAG = "fake_adc_task";
static const char* TAG2 = "processing_Task";

#define BUTTON_GPIO GPIO_NUM_0

static QueueHandle_t adc_queue;
static QueueHandle_t lo_event_queue;

static int shared_sample_count = 0;
static SemaphoreHandle_t shared_data_mutex;

void fake_adc_task(void* arg){
    const TickType_t period = pdMS_TO_TICKS(4);
    TickType_t last_wake_time = xTaskGetTickCount();

    int value = 0;

    while(1){
        if(xQueueSend(adc_queue,&value,0) != pdTRUE){
        ESP_LOGW(TAG,"ADC queue dolu, deger kayboldu: %d",value);
        }
        value++;

        vTaskDelayUntil(&last_wake_time,period);
    }

}

void processing_task(void* arg){
    int receivedVal;
    while(1){
        if(xQueueReceive(adc_queue,&receivedVal,portMAX_DELAY) == pdTRUE){
            if(receivedVal % 50 == 0){
                xSemaphoreTake(shared_data_mutex,portMAX_DELAY);
                shared_sample_count++;
                xSemaphoreGive(shared_data_mutex);

                ESP_LOGW(TAG2,"İstatistik Güncellemesi: Örnek=%d, toplam=%d",receivedVal,shared_sample_count);
            }
        }
    }
}

void IRAM_ATTR button_isr_handler(void* arg){
    uint32_t gpio_num = (uint32_t)arg;

    BaseType_t higher_priority_task_woken = pdFALSE;

    xQueueSendFromISR(lo_event_queue,&gpio_num,&higher_priority_task_woken);

    if(higher_priority_task_woken){
        portYIELD_FROM_ISR();
    }
}

void button_init(void){
    gpio_config_t io_conf  = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf ));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_GPIO,button_isr_handler,(void*)BUTTON_GPIO));
}

void lo_event_task(void* arg){
    uint32_t gpio_num;

    while(1){
        if(xQueueReceive(lo_event_queue,&gpio_num,portMAX_DELAY)==pdTRUE){
            xSemaphoreTake(shared_data_mutex,portMAX_DELAY);
            int current_count  = shared_sample_count;
            xSemaphoreGive(shared_data_mutex);

            ESP_LOGI(
                "LO_EVENT",
                "LO olayi tetiklendi, o ana kadar islenen toplam ornek: %d",
                current_count
            );
        }
    }
}

void app_main(void){
    ESP_LOGI(
        "MAIN",
        "Sistem baslatiliyor..."
    );

    adc_queue = xQueueCreate(20,sizeof(int));

     if (adc_queue == NULL)
    {
        ESP_LOGE(
            "MAIN",
            "ADC queue olusturulamadi!"
        );

        return;
    }

    lo_event_queue = xQueueCreate(10,sizeof(uint32_t));

     if (lo_event_queue == NULL)
    {
        ESP_LOGE(
            "MAIN",
            "LO queue olusturulamadi!"
        );

        return;
    }

    shared_data_mutex = xSemaphoreCreateMutex();

     if (shared_data_mutex == NULL)
    {
        ESP_LOGE(
            "MAIN",
            "Mutex olusturulamadi!"
        );

        return;
    }

    button_init();

    xTaskCreate(fake_adc_task,"fake_adc_task",2048,NULL,5,NULL);
    xTaskCreate(processing_task,"processing_task",2048,NULL,5,NULL);
    xTaskCreate(lo_event_task,"lo_event_task",2048,NULL,5,NULL);

    ESP_LOGI(
        "MAIN",
        "Tum sistem baslatildi."
    );
}