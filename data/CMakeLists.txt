idf_component_register(SRCS "TuNombreDeArchivo.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES nvs_flash mqtt vfs driver esp_timer esp_driver_tsens esp_wifi esp_netif esp_event nvs_flash lwip esp_driver_uart esp_driver_gpio)