#include "esp_adc/adc_continuous.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "ad8232_driver.h"
#include "driver/gpio.h"

    // 1) Queue oluştur: xQueueCreate(uzunluk, eleman_boyutu)
    //    NULL dönerse logla ve return et.
    // 2) ad8232_config_t değişkenini doldur.
    // 3) ad8232_init(&cfg, queue) çağır, sonucu kontrol et.
    // 4) ad8232_start() çağır, sonucu kontrol et.

static const char* TAG = "main";

static QueueHandle_t my_queue;



void app_main(void){
    my_queue = xQueueCreate(10,sizeof(uint16_t));
    if(my_queue == NULL){
        ESP_LOGE(TAG,"my_queue is Null");
        return;
    }

    ad8232_config_t ad8232_conf = {
        .output_adc_channel = ADC_CHANNEL_3,
        .lo_plus_gpio = GPIO_NUM_5,
        .lo_minus_gpio = GPIO_NUM_6,
        .adc_sample_freq_hz = 2000,
        .output_sample_freq_hz = 250
    };

    esp_err_t ret = ad8232_init(&ad8232_conf,my_queue);

    if(ret != ESP_OK){
        ESP_LOGE(TAG,"ad8232_init hatası %s",esp_err_to_name(ret));
        return;
    }

    ret = ad8232_start();
    if(ret != ESP_OK){
        ESP_LOGE(TAG,"ad8232_start hatası %s",esp_err_to_name(ret));
        return;
    }
}