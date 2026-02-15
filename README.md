# IRHub-8266

IRHub-8266 é um hub de automação baseado em **ESP8266 (NodeMCU)** com
suporte a:

-   📡 Comunicação MQTT
-   📺 Envio e recepção de sinais IR (NEC e NIKAI)
-   🌐 Servidor Web embarcado
-   🔄 Atualização OTA (Over-The-Air)
-   🖥 Debug via Telnet
-   💡 Controle de saída digital (LED)

O objetivo do projeto é atuar como ponte entre dispositivos
infravermelho e sistemas de automação como Home Assistant, Node-RED e
outros clientes MQTT.

------------------------------------------------------------------------

# 📦 Funcionalidades

## MQTT

-   Publicação de status online/offline (Last Will)
-   Publicação periódica de:
    -   Status do sistema
    -   Informações de firmware
    -   Informações de rede
    -   Estado do MQTT
    -   Uptime
    -   Estado das saídas
-   Assinatura automática de tópicos de comando

## IR

-   Envio de códigos:
    -   NEC (Decimal e Hexadecimal)
    -   NIKAI (Decimal e Hexadecimal)
-   Recepção de códigos:
    -   NEC válido
    -   NIKAI 24 bits
    -   24 bits genérico
    -   Desconhecido (modo avançado)
-   Controle de modo de envio via `typeSendCod`

## OTA

-   Atualização remota via ArduinoOTA
-   Hostname baseado no ChipID
-   Suporte a senha

## Web Server

-   Página HTTP simples exibindo uptime
-   Atualização automática a cada 2 segundos

## Telnet Debug

-   Porta 8266
-   Comandos simples:
    -   `ler`
    -   `status`

------------------------------------------------------------------------

# 🧠 Arquitetura MQTT

Base topic:

    IRHub-8266-Sala

## Tópicos de Comando

    IRHub-8266-Sala/command
    IRHub-8266-Sala/command/LEDA
    IRHub-8266-Sala/command/IR/typeSendCod
    IRHub-8266-Sala/command/IR/NEC/DEC
    IRHub-8266-Sala/command/IR/NEC/HEX
    IRHub-8266-Sala/command/IR/NIKAI/DEC
    IRHub-8266-Sala/command/IR/NIKAI/HEX

## Tópicos de Informação

    /info/status
    /info/software
    /info/network
    /info/mqtt
    /info/uptime
    /info/Outputs
    /IR/typeSendCod

## Sensores IR

    /sensores/IR/NEC
    /sensores/IR/NIKAI
    /sensores/IR/24bits
    /sensores/IR/Desconhecido

------------------------------------------------------------------------

# ⚙ Hardware

## Pinos utilizados

  Função        GPIO
  ------------- --------
  LED           GPIO2
  IR Emissor    GPIO4
  IR Receptor   GPIO14

⚠ Observações: - GPIO16 não possui interrupções. - GPIO14 pode causar
reset em ESP32-C3 (não aplicável ao ESP8266).

------------------------------------------------------------------------

# 🔐 Configuração

## WiFi

``` cpp
#define wifi_ssid "You_shall_not_pass"
#define wifi_password "felicidade42"
```

## MQTT

``` cpp
#define mqtt_server "192.168.99.15"
```

Usuário e senha configuráveis no código.

------------------------------------------------------------------------

# 🔁 Modos de Envio IR

  typeSendCod   Comportamento
  ------------- -------------------------------
  0             Não envia nada
  1             Apenas NEC
  2             NEC + 24 bits
  3             NEC + 24 bits + desconhecidos

------------------------------------------------------------------------

# 📡 Exemplo de Envio MQTT

Enviar código NEC em decimal:

    Topic: IRHub-8266-Sala/command/IR/NEC/DEC
    Payload: 551489775

Enviar código NEC em hexadecimal:

    Topic: IRHub-8266-Sala/command/IR/NEC/HEX
    Payload: 20DF10EF

------------------------------------------------------------------------

# 🔄 Inicialização

Durante o boot o dispositivo:

1.  Conecta ao WiFi
2.  Configura OTA
3.  Configura MQTT
4.  Inicia servidor HTTP
5.  Pisca LED 10 vezes
6.  Publica feedback inicial

------------------------------------------------------------------------

# 📊 Feedback Automático

Publicações periódicas:

-   Software + rede → a cada 15 minutos
-   Uptime → a cada 5 minutos

------------------------------------------------------------------------

# 🛠 Dependências

Bibliotecas utilizadas:

-   ESP8266WiFi
-   ESP8266WebServer
-   PubSubClient
-   ArduinoOTA
-   IRremoteESP8266

------------------------------------------------------------------------

# 🚀 Objetivo do Projeto

Criar um hub IR confiável, modular e preparado para integração com
sistemas de automação residencial via MQTT.

------------------------------------------------------------------------

# 📜 Licença

Este projeto pode ser distribuído sob licença MIT (ou conforme definido
no repositório).


```text
IRHub-8266/
├── README.md
├── LICENSE
├── .gitignore
│
├── lib/
│   ├── PubSubClient/
│   ├── IRremoteESP8266/
│   └── AHT10/
│
├── firmware/
│   └── IRHub-8266/
│       ├── IRHub-8266.ino
│       ├── callback.ino
│       ├── feedback.ino
│       ├── mqtt_reconnect.ino
│       ├── myIRdecoder.ino
│       ├── server.ino
│       ├── setup_ota.ino
│       ├── setup_wifi.ino
│       ├── AHT10.ino
│       ├── log.ino
│
├── docs/
│   ├── mqtt-topics.md
│   ├── hardware.md
│   ├── pinout.md
│   └── flow-diagram.png
│
├── examples/
│   ├── minimal/
│   │   └── minimal.ino
│   └── mqtt-test/
│       └── mqtt-test.ino
│
├── tools/
│   ├── mosquitto/
│   │   └── irhub-test.pub
│   └── node-red/
│       └── IRHub-flow.json
│
└── assets/
    ├── images/
    │   └── dashboard.png
    └── gifs/
        └── demo.gif

```
