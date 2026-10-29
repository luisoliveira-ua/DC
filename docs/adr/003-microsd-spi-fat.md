# ADR 003: Armazenamento local em microSD por SPI com FAT

- **Data:** 2026-10-29
- **Responsável:** Equipa do projeto DC
- **Estado:** Aceite

## Contexto

O sistema tem de guardar as leituras do sensor e os logs para consulta posterior. O protótipo usa um cartão microSD ligado ao ESP32-C6, e é necessário definir a interface de comunicação e a forma de disponibilizar os ficheiros ao firmware.

## Decisão

Armazenar os dados localmente num cartão microSD, ligado através de SPI, e montar o sistema de ficheiros FAT com as APIs do ESP-IDF (`esp_vfs_fat` e `sdmmc`).

## Justificação

- O cartão microSD oferece armazenamento persistente e amovível, adequado à recolha de dados no protótipo.
- SPI é suportado pelo modo SDSPI do ESP-IDF e usa os sinais de relógio, entrada, saída e seleção do dispositivo.
- O suporte FAT do ESP-IDF permite aceder aos dados através das operações de ficheiros comuns da biblioteca C.
- Os ficheiros podem ser retirados do cartão e consultados noutro equipamento com suporte para FAT.

## Alternativas consideradas

- **Memória flash interna:** evita um periférico amovível, mas tem capacidade limitada e requer atenção à gestão de desgaste e ao formato de armazenamento.
- **Armazenamento remoto por rede:** permitiria centralizar os dados, mas depende de conectividade e de um serviço remoto, não sendo necessário para o registo local atual.
- **Cartão SD em modo SDMMC:** é outra interface suportada pelo ESP-IDF, mas o projeto usa o modo SDSPI e a configuração de pinos SPI do hardware atual.

## Consequências

- O sistema depende da presença e montagem bem-sucedida do cartão para iniciar a persistência dos dados.
- A cablagem SPI, o pino de Chip Select e a alimentação do cartão têm de corresponder à configuração do firmware.
- A remoção ou falha do cartão pode impedir a gravação; o firmware deve tratar erros de montagem e de acesso aos ficheiros.
- O formato FAT facilita a leitura externa, mas não oferece por si só mecanismos de transação ou proteção contra corrupção causadas por falhas de alimentação.

## Referências

- Código: `proj/main/cartao.c` e `proj/main/cartao.h`.
- Documentação ESP-IDF, FAT filesystem: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/storage/fatfs.html
- Documentação ESP-IDF, SD SPI host: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/sdspi_host.html
