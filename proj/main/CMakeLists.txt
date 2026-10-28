idf_component_register(SRCS "cartao.c" "DHT_20.c" "led.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES led_strip)
