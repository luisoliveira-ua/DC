# DC — EcoNode

## Estado dos requisitos

| Req. | Descrição abreviada | Estado | Evidência de validação |
|:---:|---|:---:|---|
| **RF1** | Aquisição periódica DHT20 | em curso | [`rf1-dht20-leitura.jpeg`](docs/evidences/rf1-dht20-leitura.jpeg) — leitura testada; periodicidade pendente |
| **RF2** | Visualização no TFT | não iniciado | — |
| **RF3** | Indicação de estado pelo LED RGB | em curso | — |
| **RF4** | Registo de leituras e logs no microSD | em curso | — |
| **RF5** | Publicação de telemetria remota | não iniciado | — |
| **RF6** | Controlo do brilho do TFT | não iniciado | — |
| **RF7** | Configuração através de ficheiro JSON | em curso | — |
| **RF8** | Serviço BLE Environmental Sensing | não iniciado | — |
| **RF9** | Serviço BLE de configuração | não iniciado | — |
| **RF10** | Provisionamento por BLE | não iniciado | — |
| **RF11** | Estratégia de redução de consumo | não iniciado | — |
| **RNF1** | Distribuição por tarefas, sem bloqueio mútuo | em curso | — |
| **RNF2** | Tolerância a falhas de rede | não iniciado | — |
| **RNF3** | Tempo de resposta adequado | não iniciado | — |
| **RNF4** | Interface simples e intuitiva | não iniciado | — |
| **RNF5** | Alteração de parâmetros sem recompilação | não iniciado | — |
| **RNF6** | Segurança das credenciais e dos dados | não iniciado | — |
| **RNF7** | Interfaces públicas e independência entre módulos | em curso | — |

## Semana 1 e 2 (19/10 a 30/10)

**Requisitos trabalhados:** RF1, RF3, RF4, RF7, RNF1, RNF7.

**Trabalho realizado.** Em `proj/main/sensor/DHT_20.c`, foi implementada a comunicação I²C com o DHT20, a conversão das leituras e o envio das amostras através de um Message Buffer. A tarefa `sd_data`, em `proj/main/cartao/cartao.c`, recebe as amostras e regista data, temperatura e humidade no cartão microSD. Foi criado o módulo do LED RGB em `proj/main/led/`. Foi também criado o módulo `proj/main/config/app_config.c` para ler e validar a versão, o período de amostragem e os parâmetros atribuídos ao grupo no `config.json` da raiz do repositório.

**Decisões tomadas.** A comunicação entre aquisição e armazenamento usa Message Buffer, conforme o sorteio. As decisões sobre Message Buffer, I²C, armazenamento microSD por SPI/FAT e formato CSV estão registadas, respetivamente, nos ADRs 001 a 004 em `docs/adr/`. O guião exige pelo menos cinco ADRs, pelo que ainda falta um registo.

**Dificuldades.** A chamada a `app_config_init()` ainda não foi integrada em `app_main()`. Por isso, o período do JSON ainda não é carregado antes de iniciar a tarefa do DHT20. O ficheiro `config.json` está na raiz do repositório e ainda tem de ser copiado para a raiz do microSD, onde o firmware o procura como `/sdcard/config.json`.

**Verificações efetuadas.** Foi feita uma revisão do código. A leitura de temperatura e humidade do sensor DHT20 foi testada. A periodicidade configurável ainda não foi verificada. A iluminação de iniciação do LED RGB também foi testada.

**Divisão de trabalho.** 125016: Comunicação I²C com o DHT20, conversão das leituras e envio das amostras pelo Message Buffer; Teste de leitura do DHT20; Tarefa de armazenamento: receção das amostras e escrita de data, temperatura e humidade no microSD; Criação do módulo do LED RGB e teste básico de iluminação. 126011: Criação do config.json com os parâmetros atribuídos ao grupo; Criação do módulo de leitura e validação da configuração; Registo dos ADRs 001–004 e atualização da tabela de requisitos no README.

**Utilização de ferramentas de geração assistida.** Foi consultado um assistente para apoio na correção do código.

**Trabalho previsto para a semana seguinte.** Integrar a leitura do JSON no arranque antes da tarefa do DHT20. Copiar o ficheiro para o microSD, compilar e verificar períodos válidos e inválidos, a aquisição e a gravação das amostras. Guardar evidências em `docs/evidences/` e registar o ADR em falta.

