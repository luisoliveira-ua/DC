#ifndef LED_H
#define LED_H

#include <stdint.h>
#include "esp_err.h"

/** Inicializa o LED RGB WS2812 integrado na ESP32-C6-DevKitC-1. */
esp_err_t led_init(void);

/** Define a cor do LED; cada componente RGB aceita valores de 0 a 255. */
esp_err_t led_cor(uint8_t vermelho, uint8_t verde, uint8_t azul);

/** Apaga o LED RGB. */
esp_err_t led_apagar(void);

#endif /*LED_H */
