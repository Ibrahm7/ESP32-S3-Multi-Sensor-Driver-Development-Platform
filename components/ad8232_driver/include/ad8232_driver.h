#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdbool.h>
#include <stdint.h>


#ifndef AD8232_DRIVER_H
#define AD8232_DRIVER_H



typedef struct{
    int output_adc_channel;
    int lo_plus_gpio;
    int lo_minus_gpio;
    uint32_t sample_freq_hz; //250-500 HZ
}ad8232_config_t;

esp_err_t ad8232_init(const ad8232_config_t* config, QueueHandle_t sample_queue);

esp_err_t ad8232_start(void);
esp_err_t ad8232_stop(void);
bool ad8232_is_lead_off(void);


#endif // AD8232_DRIVER_H