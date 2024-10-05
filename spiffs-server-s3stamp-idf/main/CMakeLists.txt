set(SRCS 
    "main.c" 
    "connect_wifi.c"
    "led_strip_encoder.c"
    )
idf_component_register(SRCS ${SRCS}
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(spiffs ../data FLASH_IN_PROJECT)
