#include "DHT_20.h"
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "../config/app_config.h"
#include "../config/time.h"

static const char *TAG = "DHT20";


#define I2C_MASTER_SCL_IO           7
#define I2C_MASTER_SDA_IO           6
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000
#define dht20_ADDR                   0x38


// Handle do barramento I2C: identifica o barramento criado por i2c_new_master_bus().
i2c_master_bus_handle_t bus_handle;

// Handle do dispositivo I2C (o DHT20): identifica o sensor no barramento.
i2c_master_dev_handle_t dev_handle;

// Estrutura que armazena os valores lidos pelo sensor DHT20
dht20_data_t valores;

// Função que inicializa o barramento I2C (modo master) e adiciona o sensor DHT20 como dispositivo no barramento.
// Recebe dois ponteiros para handles, que são preenchidos por esta função:
//  - bus_handle: identifica o barramento I2C criado
//  - dev_handle: identifica o dispositivo (DHT20) ligado a esse barramento
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{    

    // Configuração do barramento I2C 
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    // Cria o barramento I2C com a configuração acima.
    // ESP_ERROR_CHECK aborta o programa e mostra o erro se a função falhar.
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    // Configuração do sensor DHT20
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dht20_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    // Adiciona o DHT20 ao barramento criado e devolve o handle do dispositivo,
    // que será usado depois nas funções de leitura/escrita (i2c_master_transmit, i2c_master_receive, etc.).
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
}

//Tarefa/função que lê periodicamente o sensor DHT20 (temperatura e humidade) por I2C
// e escreve ----
static void read_dht20(i2c_master_dev_handle_t dev_handle){

    while (1) {
        // 1) Pedir ao sensor para iniciar a medição
        // Comando de trigger do DHT20: 0xAC (comando de medição), 0x33 e 0x00 (parâmetros fixos do datasheet)
        uint8_t trigger_cmd[3] = {0xAC, 0x33, 0x00}; 

        // Envia o comando ao sensor. Timeout -1 = espera indefinidamente.
        esp_err_t ret = i2c_master_transmit(dev_handle, trigger_cmd,
                                            sizeof(trigger_cmd), 0);

        if(ret != ESP_OK){
            ESP_LOGE(TAG, "Erro na transmissão I2C: %s",esp_err_to_name(ret));
            continue;
        }
        

        // O sensor precisa de tempo para medir (o datasheet indica 80 ms)
        vTaskDelay(pdMS_TO_TICKS(80));
        
        //  2) Ler a resposta (7 bytes)
        // Formato: [0] estado | [1..3] humidade + parte da temperatura | [4..5] temperatura | [6] CRC
        uint8_t rx_data[7] = {0}; 
        ret = i2c_master_receive(dev_handle, rx_data, sizeof(rx_data), 0);

         if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Erro na receção I2C: %s",esp_err_to_name(ret));
            continue;
        }
        
        // Bit 7 do byte de estado: 1 = sensor ocupado a medir, 0 = medição pronta
        if ((rx_data[0] & 0x80) == 0) { 

            // 3) Extrair os valores brutos (20 bits cada) 
            // Humidade: bytes 1 e 2 completos + 4 bits mais altos do byte 3
            uint32_t raw_humidity =
                                    ((uint32_t)rx_data[1] << 12) |
                                    ((uint32_t)rx_data[2] << 4) |
                                    ((rx_data[3] >> 4));

            // Temperatura: 4 bits mais baixos do byte 3 + bytes 4 e 5 completos
            uint32_t raw_temperature =
                                    ((uint32_t)(rx_data[3] & 0x0F) << 16) |
                                    ((uint32_t)rx_data[4] << 8) |
                                    rx_data[5];

            //4) Converter para unidades reais (fórmulas do datasheet)
            valores.humidade = ((float)raw_humidity / 1048576.0f) * 100.0f;
            valores.temperatura = ((float)raw_temperature / 1048576.0f) * 200.0f - 50.0f;
            valores.time=time_now();

            size_t enviados = xMessageBufferSend(
                            buffer_amostras,
                            &valores,
                            sizeof(valores),
                            pdMS_TO_TICKS(0));
            
            if(enviados != sizeof(valores)){
               ESP_LOGE(TAG, "Erro no envio no buffer: %s",esp_err_to_name(ret));
               continue;
        }
            
     
        }
        else { 
            // Sensor ainda ocupado: descarta esta leitura e tenta no próximo ciclo
            ESP_LOGW(TAG, "Sensor ocupado, medição ignorada."); 
        }

        // O período é consultado no módulo de configuração em cada ciclo.
        vTaskDelay(pdMS_TO_TICKS(app_config_get_periodo_s() * 1000U));
    }
}






void dht20(void)
{
    // Guarda o byte de estado lido do sensor
    uint8_t status;


    // 1) Inicializar o I2C
    // Cria o barramento e adiciona o DHT20; preenche bus_handle e dev_handle
    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG, "I2C initialized successfully");
    
    // O datasheet exige ~100 ms de espera após ligar o sensor, antes de comunicar
    vTaskDelay(pdMS_TO_TICKS(100));

    // 2) Verificar o estado de calibração
    // 0x71 é o comando para ler o registo de estado do DHT20
    uint8_t cmd_status = 0x71;

    // Envia 1 byte (o comando) e lê 1 byte (o estado) numa só transação I2C
    ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, &cmd_status, 1, &status, 1, -1)); 
    
    // Verifica os bits 4 e 3 (0x18) do estado. O datasheet pede (status & 0x18) == 0x18
    // para considerar o sensor calibrado e pronto
    if ((status & 0x18) != 0x18) {
         ESP_LOGW(TAG, "Sensor não calibrado! Estado: 0x%02X", status);
    }
     else { 
        ESP_LOGI(TAG, "Sensor calibrado corretamente. Estado: 0x%02X", status);
    }

    // Pequena pausa antes de iniciar as medições
    vTaskDelay(pdMS_TO_TICKS(10));

    //3) Ciclo de leitura
    // Lê temperatura e humidade periodicamente.
    read_dht20(dev_handle);

    //4) Libertar recursos
    // Remove o dispositivo (DHT20) do barramento
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));

    // Apaga o barramento I2C e liberta o periférico
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C de-initialized successfully");
}
