#ifndef CARTAO_H
#define CARTAO_H

#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "sdmmc_cmd.h"
#include "freertos/message_buffer.h"



/* ----------------------------- Definições ----------------------------- */

#define MAX_CHAR_SIZE   64                      // tamanho máximo de buffers de texto
#define INPUT_GPIO      GPIO_NUM_9              // pino do botão BOOT
#define MOUNT_POINT     "/sdcard"               // ponto de montagem do cartão no VFS
#define LOG_FILE        MOUNT_POINT "/log.txt"  // ficheiro de registo
#define LEITURA_FILE    MOUNT_POINT "/leitura.txt"
#define PERIODO_MS      1000                    // período de registo (ms)

// Pinos SPI definidos via menuconfig
#define PIN_NUM_MISO    CONFIG_EXAMPLE_PIN_MISO
#define PIN_NUM_MOSI    CONFIG_EXAMPLE_PIN_MOSI
#define PIN_NUM_CLK     CONFIG_EXAMPLE_PIN_CLK
#define PIN_NUM_CS      CONFIG_EXAMPLE_PIN_CS

/* ------------------------- Variáveis partilhadas ----------------------- */

extern volatile bool parar;                     // posta a true pela ISR do botão
extern sdmmc_card_t *card;                      // descritor do cartão SD
extern MessageBufferHandle_t buffer_amostras;   // amostras recebidas do sensor

/* ------------------------------- Funções ------------------------------- */

esp_err_t cartao_init(void);




#endif
