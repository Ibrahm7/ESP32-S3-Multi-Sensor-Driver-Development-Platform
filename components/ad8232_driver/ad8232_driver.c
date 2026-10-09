#include "ad8232_driver.h"
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"


#define ESP32_MAX_FREQ_HZ 83333
#define ESP32_MIN_FREQ_HZ 611

static const char *TAG = "AD8232";

static adc_continuous_handle_t adc_handle;
static uint32_t decimation_ratio;
static QueueHandle_t queue;
static TaskHandle_t ad8232_task_handle = NULL;

esp_err_t ad8232_init(const ad8232_config_t *config, QueueHandle_t sample_queue)
{
    queue = sample_queue;


    if(config == NULL || sample_queue == NULL){
        ESP_LOGE(TAG, "config veya sample_queue NULL!");
        return ESP_ERR_INVALID_ARG;
    }

    if(config->adc_sample_freq_hz < ESP32_MIN_FREQ_HZ || config->adc_sample_freq_hz > ESP32_MAX_FREQ_HZ){
        ESP_LOGE(TAG,"Geçersiz ADC Örnekleme Hızı: %lu Hz",(unsigned long)config->adc_sample_freq_hz);
        return ESP_ERR_INVALID_ARG;
    }

    if(config->output_sample_freq_hz == 0){
        ESP_LOGE(TAG,"Çıkış örnekleme hızı sıfır olamaz.");
        return ESP_ERR_INVALID_ARG;
    }

    if(config->adc_sample_freq_hz % config->output_sample_freq_hz != 0){
        ESP_LOGE(TAG,
                 "ADC hizi (%lu Hz), cikis hizina (%lu Hz) tam bolunmuyor!",
                 (unsigned long)config->adc_sample_freq_hz,
                 (unsigned long)config->output_sample_freq_hz);
        return ESP_ERR_INVALID_ARG;
    }

    decimation_ratio = config->adc_sample_freq_hz / config->output_sample_freq_hz;

    adc_continuous_handle_cfg_t mem_config = {
        .max_store_buf_size = 1024, // DMA'nın doldurduğu toplam havuz (byte)
        .conv_frame_size = 256      // tek seferde okuyacağın çerçeve (byte)
    };

    esp_err_t ret = adc_continuous_new_handle(&mem_config,&adc_handle);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "mem_config basarisiz: %s", esp_err_to_name(ret));
        return ret;
    }
    
    adc_digi_pattern_config_t pat = {
        .atten = ADC_ATTEN_DB_12,
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
        .channel = config->output_adc_channel,
        .unit = ADC_UNIT_1
    };

    adc_continuous_config_t adc_config = {
        .adc_pattern = &pat,
        .sample_freq_hz = config->adc_sample_freq_hz,
        .pattern_num = 1,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2
    };

    ret = adc_continuous_config(adc_handle,&adc_config);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "config basarisiz: %s", esp_err_to_name(ret));
        adc_continuous_deinit(adc_handle); //// yukarıda ayrılanı geri ver
        adc_handle = NULL;
        return ret;
    }


    return ESP_OK;
}

static void ad8232_read_task(void* param){
    uint8_t buffer[256]; //ADC sürücüsünden okuduğun verileri saklayacağın yer burası.
    uint32_t out_length  = 0;
    while(1){
        esp_err_t ret = adc_continuous_read(adc_handle,buffer,sizeof(buffer),&out_length,1000);
        if(ret == ESP_OK){
            //Burada okunan byte sayısını, kaç ADC örneğine karşılık geldiğine çeviriyorsun.
            uint32_t sample_count = out_length / sizeof(adc_digi_output_data_t);

            if(sample_count > 0){
                adc_digi_output_data_t* p = (adc_digi_output_data_t*)buffer;
                ESP_LOGI(TAG,"Okunan veri: %lu byte, ilk ADC degeri: %u",(unsigned long)out_length,(unsigned int)p->type2.data);
            }
        }else if(ret == ESP_ERR_TIMEOUT){
            continue;
        }else{
             ESP_LOGE(TAG, "ADC okuma hatasi: %s", esp_err_to_name(ret));
        }
    }
}


esp_err_t ad8232_start(void)
{   
    if(adc_handle == NULL){
        ESP_LOGE(TAG,"adc_handle NULL olmamalı.");
        return ESP_ERR_INVALID_STATE; //fonksiyonun mevcut durumda çalıştırılamadığını ifade eder.
    }

     // Zaten bir okuma task'i varsa ikinci kez başlatma.
    if (ad8232_task_handle != NULL) {
        ESP_LOGE(TAG, "AD8232 okuma task'i zaten calisiyor.");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = adc_continuous_start(adc_handle);
    if(ret != ESP_OK){
        ESP_LOGE(TAG,"adc_handle başlatılamadı %s.",esp_err_to_name(ret));
        return ret;
    }

    BaseType_t task_ret = xTaskCreate(ad8232_read_task,"ad8232_read_task",4096,NULL,5,&ad8232_task_handle);

    if(task_ret != pdPASS){
        ESP_LOGE(TAG, "Okuma task'i olusturulamadi.");

        // Task oluşturulamadığı için ADC'yi geri durdur.
        ret = adc_continuous_stop(adc_handle);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ADC geri durdurulamadi: %s",
                     esp_err_to_name(ret));
        }

        ad8232_task_handle = NULL;
        return ESP_ERR_NO_MEM;
    }


    return ESP_OK;
}

esp_err_t ad8232_stop(void)
{
    if(adc_handle == NULL){
        ESP_LOGE(TAG,"adc_handle NULL olmamalı.");
        return ESP_ERR_INVALID_STATE;
    }

    if (ad8232_task_handle == NULL) {
        ESP_LOGE(TAG, "AD8232 okuma task'i calismiyor.");
        return ESP_ERR_INVALID_STATE;
    }


    esp_err_t ret = adc_continuous_stop(adc_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC durdurulamadi: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    // Task'in artık ADC okumaya devam etmesini engelle.
    vTaskDelete(ad8232_task_handle);
    ad8232_task_handle = NULL;

    return ESP_OK;
}
