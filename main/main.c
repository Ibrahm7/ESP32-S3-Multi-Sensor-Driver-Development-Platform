#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char* TAG = "AD8232";

#define AD8232_ADC_CHANNEL ADC_CHANNEL_3 //GPIO4
#define LO_PLUS_PIN  GPIO_NUM_5  // senin bagladigin pine gore degistir
#define LO_MINUS_PIN GPIO_NUM_6
static adc_oneshot_unit_handle_t adc_handle;

void ad8232_adc_init(void){
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config,&adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    gpio_config_t lo_conf = {
    .pin_bit_mask = (1ULL << LO_PLUS_PIN) | (1ULL << LO_MINUS_PIN),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&lo_conf);

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle,AD8232_ADC_CHANNEL,&chan_config));
}

void ad8232_sampling_task(void* pvParam){
    const TickType_t period = pdMS_TO_TICKS(4);
    TickType_t last_wake_time = xTaskGetTickCount();

    int raw;

    while(1){
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle,AD8232_ADC_CHANNEL,&raw));
        int lo_plus = gpio_get_level(LO_PLUS_PIN);
        int lo_minus = gpio_get_level(LO_MINUS_PIN);
        ESP_LOGI(TAG, "Ham: %d | LO+: %d | LO-: %d", raw, lo_plus, lo_minus);

        vTaskDelayUntil(&last_wake_time,period);
    }
}

void app_main(void){
    ad8232_adc_init();

    xTaskCreate(ad8232_sampling_task,"ad8232_sampling",4096,NULL,6,NULL);
}