idf_component_register(SRCS "main.c" "http_client.c" "sensor.c" "wifi.c"
                    PRIV_REQUIRES esp_adc esp_http_client esp_wifi nvs_flash esp_event esp_netif
                    INCLUDE_DIRS ".")
