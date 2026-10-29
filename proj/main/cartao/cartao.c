#include "cartao.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>               // stat(): verificar se um ficheiro existe
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/task.h"
#include "DHT_20.h"
#include "driver/spi_common.h"      // inicialização do barramento SPI
#include "esp_timer.h"              // temporizador de alta resolução (para "millis")
#include "esp_log.h"                // ESP_LOGI / ESP_LOGE
#include "esp_vfs_fat.h"            // montagem do FAT sobre SD
#include "sdmmc_cmd.h"              // funções do cartão (ex.: sdmmc_card_print_info)
#include "time.h"

// ----------------------------- Definições -----------------------------

// ------------------------- Variáveis globais --------------------------

static const char *TAG = "Cartao";             // etiqueta usada nos logs

sdmmc_card_t *card = NULL;                      // descritor do cartão, preenchido ao montar
static sdmmc_host_t host = SDSPI_HOST_DEFAULT();// configuração por defeito do host SD sobre SPI


static vprintf_like_t original_vprintf = NULL;
static SemaphoreHandle_t log_mutex = NULL;

// ---------------------- Inicialização de SPI e SD ---------------------

static void sd_data(void *arg)
{
    dht20_data_t valores;
    const size_t tamanho = sizeof(valores);

    for (;;) {
        size_t recebido = xMessageBufferReceive(
            buffer_amostras,
            &valores,
            tamanho,
            portMAX_DELAY
        );

        if (recebido != tamanho) {
            ESP_LOGE("storage", "Amostra recebida incompleta");
            continue;
        }

        if (xSemaphoreTake(log_mutex, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        FILE *ficheiro = fopen(LEITURA_FILE, "a");
        if (ficheiro == NULL) {
            xSemaphoreGive(log_mutex);
            ESP_LOGE("storage", "Não foi possível abrir o ficheiro no SD");
            continue;
        }

        time_t instante = (time_t)valores.time;
        struct tm data;

        localtime_r(&instante, &data);

        fprintf(ficheiro, "%04d-%02d-%02dT%02d:%02d:%02d,%0.2f,%0.2f\n",
            data.tm_year + 1900,
            data.tm_mon + 1,
            data.tm_mday,
            data.tm_hour,
            data.tm_min,
            data.tm_sec,
            valores.temperatura,
            valores.humidade);
        fclose(ficheiro);
        xSemaphoreGive(log_mutex);
    }
}

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


esp_err_t cartao_init(void){
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
        return ret;                             // sem barramento não há nada a fazer
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
        return ret;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    sdmmc_card_print_info(stdout, card);        // imprime informação do cartão (tipo, capacidade, etc.)
    log_mutex = xSemaphoreCreateMutex();
    if (log_mutex == NULL) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
        card = NULL;
        spi_bus_free(host.slot);
        return ESP_ERR_NO_MEM;
    }

    // Cabeçalho apenas se o ficheiro ainda não existe
    struct stat st;
    if (stat(LOG_FILE, &st) != 0) {             // stat != 0 => o ficheiro não existe
        FILE *f = fopen(LOG_FILE, "w");         // cria o ficheiro para escrita
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
            esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card); // desmonta o cartão
            spi_bus_free(host.slot);                       // liberta o barramento SPI
            card = NULL;
            vSemaphoreDelete(log_mutex);
            log_mutex = NULL;
            return ESP_FAIL;
        }
        fputs("millis,counter\n", f);           // linha de cabeçalho do CSV
        fclose(f);                              // fecha o ficheiro (garante a escrita no cartão)
    }

    if (stat(LEITURA_FILE, &st) != 0) {             // stat != 0 => o ficheiro não existe
        FILE *f = fopen(LEITURA_FILE, "w");         // cria o ficheiro para escrita
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
            esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card); // desmonta o cartão
            spi_bus_free(host.slot);                       // liberta o barramento SPI
            card = NULL;
            vSemaphoreDelete(log_mutex);
            log_mutex = NULL;
            return ESP_FAIL;
        }
        fputs("data_hora,temperatura,humidade\n", f);    // cabeçalho do CSV
        fclose(f);                              // fecha o ficheiro (garante a escrita no cartão)
    }

    if (log_mutex != NULL) {
        original_vprintf = esp_log_set_vprintf(sd_log_output);
    }

    if (buffer_amostras == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xTaskCreate(sd_data, "sd_data", 4096, NULL, 5, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
