idf_component_register(
    SRCS "ad8232_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_adc esp_driver_gpio
)