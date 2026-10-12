idf_component_register(SRCS "main.c" 
                    INCLUDE_DIRS "."
                    REQUIRES ad8232_driver esp_driver_gpio esp_timer esp_adc )