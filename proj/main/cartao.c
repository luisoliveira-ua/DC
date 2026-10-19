#include "cartao.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>               // stat(): verificar se um ficheiro existe
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"            // configuração de GPIO e interrupções
#include "driver/spi_common.h"      // inicialização do barramento SPI
#include "esp_timer.h"              // temporizador de alta resolução (para "millis")
#include "esp_log.h"                // ESP_LOGI / ESP_LOGE
#include "esp_vfs_fat.h"            // montagem do FAT sobre SD
#include "sdmmc_cmd.h"              // funções do cartão (ex.: sdmmc_card_print_info)

// ----------------------------- Definições -----------------------------

#define MAX_CHAR_SIZE   64                      // tamanho máximo de buffers de texto
#define MOUNT_POINT     "/sdcard"               // ponto de montagem do cartão no VFS
#define LOG_FILE        MOUNT_POINT "/log.txt"  // caminho do ficheiro de registo
#define PERIODO_MS      1000                    // período de registo (1 s)

// Pinos SPI definidos via menuconfig (idf.py menuconfig)
#define PIN_NUM_MISO    CONFIG_EXAMPLE_PIN_MISO // dados do cartão -> ESP32
#define PIN_NUM_MOSI    CONFIG_EXAMPLE_PIN_MOSI // dados do ESP32 -> cartão
#define PIN_NUM_CLK     CONFIG_EXAMPLE_PIN_CLK  // relógio SPI
#define PIN_NUM_CS      CONFIG_EXAMPLE_PIN_CS   // chip select do cartão

// ------------------------- Variáveis globais --------------------------

static const char *TAG = "example";             // etiqueta usada nos logs

static volatile bool parar = false;             // posta a true pela ISR do botão
                                                // (volatile: é alterada fora do fluxo normal)
static sdmmc_card_t *card = NULL;               // descritor do cartão, preenchido ao montar
static sdmmc_host_t host = SDSPI_HOST_DEFAULT();// configuração por defeito do host SD sobre SPI


static vprintf_like_t original_vprintf = NULL;
static SemaphoreHandle_t log_mutex = NULL;

// ---------------------- Inicialização de SPI e SD ---------------------


static int sd_log_output(const char *format, va_list args)
{
    // Guardar o log no cartão SD
    if (log_mutex != NULL &&
        xSemaphoreTake(log_mutex, portMAX_DELAY) == pdTRUE) {

        FILE *file = fopen(LOG_FILE, "a");

        if (file != NULL) {
            va_list sd_args;
            va_copy(sd_args, args);

            vfprintf(file, format, sd_args);

            va_end(sd_args);
            fclose(file);
        }

        xSemaphoreGive(log_mutex);
    }

    // Encaminhar o mesmo log para o destino anterior (UART)
    if (original_vprintf != NULL) {
        va_list uart_args;
        va_copy(uart_args, args);

        int result = original_vprintf(format, uart_args);

        va_end(uart_args);
        return result;
    }

    return 0;
}


static void init_spi(){
    esp_err_t ret;                              // guarda os códigos de erro das chamadas ESP-IDF


    // Opções de montagem do sistema de ficheiros FAT
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,         // formata o cartão se a montagem falhar
#else
        .format_if_mount_failed = false,        // não formata (mais seguro)
#endif
        .max_files = 5,                         // máximo de ficheiros abertos em simultâneo
        .allocation_unit_size = 16 * 1024       // tamanho da unidade de alocação (usado ao formatar)
    };

    ESP_LOGI(TAG, "Initializing SD card (SPI)");
    host.unaligned_multi_block_rw_max_chunk_size = 8; // tamanho máximo de bloco em leituras/escritas não alinhadas

    // Configuração do barramento SPI
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,                    // -1 = pino não usado (modo quad SPI desativado)
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,                // tamanho máximo de transferência por operação (bytes)
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA); // inicializa o SPI com DMA
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;                                 // sem barramento não há nada a fazer
    }

    // Configuração do dispositivo SD no barramento
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;           // pino de chip select
    slot_config.host_id = host.slot;            // barramento SPI a utilizar

    ESP_LOGI(TAG, "Mounting filesystem");
    // Monta o FAT do cartão em MOUNT_POINT; preenche "card" em caso de sucesso
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            // O cartão respondeu, mas o sistema de ficheiros não pôde ser montado
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "Set CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED to format the card.");
        } else {
            // O cartão nem sequer foi inicializado (cablagem, pull-ups, etc.)
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        spi_bus_free(host.slot);                // liberta o barramento SPI
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    sdmmc_card_print_info(stdout, card);        // imprime informação do cartão (tipo, capacidade, etc.)
}


void app_main(void)
{


    esp_err_t ret = ESP_ERROR_CHECK(init_spi());

    if (ret != ESP_OK) {
        ESP_LOGE("erro ao iniciar o cartão")
        return;
    }


    log_mutex = xSemaphoreCreateMutex();





    // Cabeçalho apenas se o ficheiro ainda não existe
    struct stat st;
    if (stat(LOG_FILE, &st) != 0) {             // stat != 0 => o ficheiro não existe
        FILE *f = fopen(LOG_FILE, "w");         // cria o ficheiro para escrita
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
            esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card); // desmonta o cartão
            spi_bus_free(host.slot);                       // liberta o barramento SPI
            return;
        }
        fputs("millis,counter\n", f);           // linha de cabeçalho do CSV
        fclose(f);                              // fecha o ficheiro (garante a escrita no cartão)
    }

    if (log_mutex != NULL) {
        original_vprintf = esp_log_set_vprintf(sd_log_output);
    }

    
}