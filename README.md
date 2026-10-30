# DC

## Semana 1 e 2 (19/10 a 30/10)

**Requisitos trabalhados:** RF1, RF3, RF4, RF7

**Trabalho realizado.** Em `proj/main/DHT_20.c`, foi implementada a comunicação I2C com o DHT20, a conversão das leituras de temperatura e humidade e o envio dos valores para um Message Buffer. Em `proj/main/cartao.c`, foi criada uma tarefa que recebe amostras do buffer e acrescenta os valores ao ficheiro `leitura.txt` no microSD. Foi também criado o módulo `led.c`/`led.h` para controlar o LED RGB integrado da ESP32-C6-DevKitC-1.

**Decisões tomadas.** Foi sorteado o Message Buffer para passar as amostras do sensor para a tarefa de armazenamento. A decisão e as alternativas consideradas ainda devem ser registadas num ADR em `docs/adr/`.

**Dificuldades.** A integração entre o sensor e o armazenamento está pendente: a função de leitura ainda não está iniciada como tarefa FreeRTOS, e o buffer aparece definido em mais do que um ficheiro. O período de leitura está fixo em 2 segundos e ainda não é carregado de um ficheiro de configuração. Não foi feita uma compilação nem uma verificação em hardware nesta semana, devido a problemas com a breadboard.

**Verificações efetuadas.** Foi feita uma revisão do código, mas ainda não foram realizadas verificações no ESP32 nem medições de periodicidade ou escrita no cartão.

**Divisão de trabalho.** 125016: ; 126011:

**Utilização de ferramentas de geração assistida.** Consulta a um assistente para corrigir código.

**Trabalho previsto para a semana seguinte.** Corrigir a definição partilhada do Message Buffer e iniciar a tarefa de aquisição do DHT20. Compilar e testar a passagem de amostras até ao ficheiro CSV no microSD. Registar a decisão sobre o Message Buffer num ADR.
