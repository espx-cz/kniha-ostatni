set(SRCS "main.c" "connect_wifi.c")
idf_component_register(SRCS ${SRCS}
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)
