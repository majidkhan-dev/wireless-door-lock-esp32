idf_component_register(SRCS "inside-node.c" "lcd_i2c.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp_wifi esp_event esp_netif esp_rom)
