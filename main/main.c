#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ad8232_driver.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Merhaba ESP32-S3!");
    ad8232_driver_init();

    while (1) {
        ESP_LOGI(TAG, "calisiyor...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}