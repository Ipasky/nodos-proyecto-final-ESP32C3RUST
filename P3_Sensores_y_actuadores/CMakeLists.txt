idf_component_register(SRCS "p3_GPS.c" "p3_LSM6DS33.c" "gps.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_driver_tsens esp_wifi esp_netif esp_event nvs_flash lwip esp_driver_uart esp_driver_gpio
                       )