# IRHub-8266

IRHub-8266 é um hub de automação baseado em **ESP8266 (NodeMCU)** com suporte a:

- 📡 Comunicação MQTT
- 📺 Envio e recepção de sinais IR (múltiplos protocolos)
- 🌡 Sensor de temperatura e umidade (AHT10)
- 🌐 Servidor Web embarcado com frontend em LittleFS
- 🔌 WebSocket para comunicação em tempo real com o frontend
- 🔄 Atualização OTA (Over-The-Air)
- 🖥 Debug via Telnet (porta 8266)
- 💡 Controle de saída digital (LED)
- ⚙️ Configuração via portal WiFiManager + página de settings

O objetivo do projeto é atuar como ponte entre dispositivos infravermelhos e sistemas de automação como Home Assistant, Node-RED e outros clientes MQTT.

---

## 📦 Funcionalidades

### MQTT

- Publicação de status online/offline via Last Will (retain)
- Publicação periódica de uptime, rede e sensores (a cada 5 minutos)
- Publicação event-driven de IR recebido/enviado e estado do LED
- Subscrição em `<topic>/command` para receber comandos JSON
- Reconexão automática com intervalo de 60 segundos

### IR

- **Envio** de códigos pelos protocolos: NEC, SONY, RC5, RC6, SAMSUNG, NIKAI, LG, JVC, WHYNTER
- Aceita código em decimal ou hexadecimal
- **Recepção** configurável com 12 modos: ALL, KNOWN, DISABLED, e cada protocolo individualmente
- Modo de recepção persistido em `config.json`
- Modo de teste do emissor: ciclo de desligamento universal automático
- Debounce de 300ms na recepção

### Sensor AHT10

- Temperatura e umidade via I2C (SDA: GPIO12, SCL: GPIO13)
- Habilitação configurável em runtime via `/settings` (sem necessidade de reboot)
- Quando desabilitado: nenhuma leitura I2C, nenhuma publicação MQTT/WS, card oculto na UI
- Quando habilitado: tentativa de reinicialização automática a cada 60s em caso de falha
- Publicação periódica via MQTT e WebSocket (somente quando habilitado)

### Web Server / Frontend

- Frontend servido do LittleFS (HTML/CSS/JS separados)
- Páginas: `/` (controle IR), `/system` (status), `/settings` (configuração), `/files` (file manager)
- Comunicação em tempo real via WebSocket (porta 81)
- Reconexão automática do WebSocket com overlay visual
- Controle remoto virtual com modelos configuráveis via `remotes.json`
- Histórico dos últimos 10 sinais IR recebidos
- Autenticação HTTP Basic nas rotas destrutivas (`/files`, `/delete`, `/download`, `/upload`)

### WiFiManager

- Portal de configuração automático na primeira inicialização
- Parâmetros configuráveis pelo portal: hostname, MQTT ID, grupo, IP fixo/DHCP, MQTT
- Botão físico (GPIO0): pressão 1–3s abre portal, pressão >5s faz reset total
- Watchdog de reconexão WiFi a cada 30s
- Suporte a IP fixo com reaplique automático após reconexão

### OTA

- Atualização remota via ArduinoOTA
- Hostname: `clientID` (ex: `IRHub-8266-Sala-<chipID>`)
- Senha: ChipID em hexadecimal (8 dígitos)

### Telnet

- Porta 8266
- Comandos: `status`, `led`, `ir`, `ir receptor <modo>`, `irteste [on/off]`, `aht10`, `mqtt`, `network`, `info`, `heap`, `reboot`, `help`

---

## ⚙️ Hardware

### Pinos utilizados

| Função         | GPIO    | NodeMCU |
|----------------|---------|---------|
| Botão Reset    | GPIO0   | D3      |
| LED            | GPIO2   | —       |
| IR Emissor     | GPIO4   | D2      |
| AHT10 SDA      | GPIO12  | D6      |
| AHT10 SCL      | GPIO13  | D7      |
| IR Receptor    | GPIO14  | D5      |

---

## 🔐 Configuração

A configuração é feita pelo portal WiFiManager (primeira inicialização ou pressão do botão) ou pela página `/settings` do frontend. Os parâmetros são persistidos em `/config.json` no LittleFS.

### Parâmetros disponíveis

| Parâmetro     | Padrão       | Descrição                        |
|---------------|--------------|----------------------------------|
| Hostname      | `irhub8266`  | Nome mDNS (`hostname.local`)     |
| MQTT ID       | `IRHub-8266` | Prefixo do tópico MQTT           |
| Grupo         | `Sala`       | Sufixo do tópico MQTT            |
| IP / GW / SN  | —            | IP fixo (vazio = DHCP)           |
| MQTT Server   | `mqtt.local` | Endereço do broker               |
| MQTT Port     | `1883`       | Porta do broker                  |
| MQTT User     | —            | Usuário MQTT                     |
| MQTT Password | —            | Senha MQTT                       |
| MQTT Enabled  | `no`         | Habilita/desabilita MQTT         |
| AHT10 Enabled | `false`      | Habilita/desabilita sensor AHT10 |

### Senhas padrão

| Acesso           | Usuário  | Senha                    |
|------------------|----------|--------------------------|
| HTTP Basic Auth  | `admin`  | ChipID em hex (8 dígitos)|
| OTA              | —        | ChipID em hex (8 dígitos)|
| Portal WiFi      | —        | `12345678`               |

> A senha HTTP e OTA pode ser consultada via Telnet com o comando `senha`.

---

## 🧠 Arquitetura MQTT

Tópico base: `<mqtt_id>-<grupo>` (ex: `IRHub-8266-Sala`)

```
IRHub-8266-Sala/
├── command                       ← subscription (entrada)
├── status                        ← birth/will (retain)
├── info/
│   ├── device
│   ├── network
│   ├── mqtt
│   └── uptime
├── switch/
│   └── led/state
└── sensor/
    ├── aht10/state
    ├── aht10/status
    └── ir/
        ├── config/state
        ├── received/state
        └── sent/state
```

Para detalhes completos sobre payloads e comandos, consulte [`docs/MQTT.md`](docs/MQTT.md).

---

## 🔄 Inicialização

Durante o boot o dispositivo executa na ordem:

1. Monta LittleFS e carrega `config.json`
2. Conecta ao WiFi via WiFiManager (abre portal se necessário)
3. Inicia mDNS
4. Configura OTA
5. Configura e conecta ao MQTT (se habilitado)
6. Inicia servidor HTTP (porta 80) e WebSocket (porta 81)
7. Inicializa IR (emissor GPIO4, receptor GPIO14)
8. Inicializa AHT10 (I2C GPIO12/13, somente se habilitado em `config.json`)
9. Configura botão de reset (GPIO0)
10. Pisca LED 5 vezes (feedback de boot)

---

## 📁 Estrutura de arquivos

### Firmware (Arduino)

```
IRHub-8266/
├── IRHub-8266.ino     — setup/loop
├── globals.h          — declarações externas e enums
├── globals.cpp        — variáveis globais, LED, senhas
├── Network.ino        — WiFi e watchdog
├── Config.ino         — load/save/reset config
├── ServerWS.ino       — HTTP + WebSocket
├── MQTT.ino           — conexão MQTT, publishers
├── IR.ino             — emissor, receptor, parser, feedback
├── AHT10.ino          — sensor AHT10
├── OTA.ino            — atualização OTA
├── Telnet.ino         — servidor telnet + comandos
├── Debug.ino          — funções de debug
```

### Frontend (LittleFS)

```
data/
├── index.html         — página principal (controle IR)
├── system.html        — status do sistema
├── settings.html      — configuração do dispositivo
├── app.js             — lógica compartilhada (WebSocket, UI)
├── style.css          — estilos globais
└── remotes.json       — modelos de controle remoto
```

---

## 🛠 Dependências

| Biblioteca           | Uso                        |
|----------------------|----------------------------|
| ESP8266WiFi          | WiFi                       |
| ESP8266WebServer     | Servidor HTTP              |
| ESP8266mDNS          | mDNS                       |
| WiFiManager          | Portal de configuração     |
| WebSocketsServer     | WebSocket (porta 81)       |
| PubSubClient         | MQTT                       |
| ArduinoOTA           | Atualização OTA            |
| IRremoteESP8266      | Envio e recepção IR        |
| Adafruit_AHTX0       | Sensor AHT10               |
| ArduinoJson          | Serialização JSON          |
| LittleFS             | Sistema de arquivos        |

---

## 🚀 Como usar

1. Grave o firmware via Arduino IDE
2. Faça upload dos arquivos da pasta `data/` via LittleFS (Arduino IDE → Tools → ESP8266 LittleFS Data Upload)
3. Na primeira inicialização, conecte-se à rede `irhub8266` e acesse `192.168.4.1` para configurar WiFi e MQTT
4. Após conectar, acesse `http://irhub8266.local` ou pelo IP atribuído
5. Use a página `/settings` para ajustar configurações sem precisar do portal

> Após qualquer upload de arquivos LittleFS, faça **Ctrl+Shift+R** no browser para limpar o cache.
