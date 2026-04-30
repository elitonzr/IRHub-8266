# IRHub-8266

IRHub-8266 é um hub de automação baseado em **ESP8266 (NodeMCU)** com suporte a:

- 📡 Comunicação MQTT
- 📺 Envio e recepção de sinais IR (múltiplos protocolos)
- 🌡 Sensor de temperatura e umidade (AHT10)
- 🌐 Servidor Web embarcado com frontend em LittleFS
- 🔌 WebSocket para comunicação em tempo real com o frontend
- 🔄 Atualização OTA (Over-The-Air)
- 🖥 Debug via Telnet (porta 8266)
- 💡 Controle de saída digital (LED B)
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
- Aceita código em decimal ou hexadecimal (com ou sem prefixo `0x`)
- Suporte a códigos de até 64 bits
- **Recepção** configurável com 12 modos: ALL, KNOWN, DISABLED e cada protocolo individualmente
- Modo de recepção persistido em `config.json`
- Modo de teste do emissor: ciclo de desligamento universal automático
- Debounce de 300ms na recepção

### Sensor AHT10

- Temperatura e umidade via I2C (SDA: GPIO12/D6, SCL: GPIO13/D7)
- Habilitação configurável em runtime via `/settings` (sem necessidade de reboot)
- Quando desabilitado: nenhuma leitura I2C, nenhuma publicação MQTT/WS, card oculto na UI
- Quando habilitado: tentativa de reinicialização automática a cada 60s em caso de falha
- Publicação periódica via MQTT e WebSocket (somente quando habilitado)

### Web Server / Frontend

- Frontend SPA (Single Page Application) servido do LittleFS
- Páginas: `/` (controle IR), `/ir` (receptor/emissor), `/system` (status), `/settings` (configuração), `/files` (file manager)
- Comunicação em tempo real via WebSocket (porta 81)
- Reconexão automática do WebSocket com overlay visual
- Controle remoto virtual com modelos configuráveis via `remotes.json`
- Histórico dos últimos 10 sinais IR recebidos
- Autenticação HTTP Basic nas rotas de arquivo (`/files`, `/delete`, `/download`, `/upload`, `/update`)
- Autenticação WebSocket por login com usuário e senha — exigida para acesso às páginas `/ir`, `/system` e `/settings` e para ações de configuração
- Rotas públicas (sem autenticação): `/` (apenas card Controle IR), `/app.js`, `/style.css`, `/remotes.json`, WebSocket (leitura de estado e envio IR)

### WiFiManager

- Portal de configuração automático na primeira inicialização
- Parâmetros configuráveis pelo portal: hostname, MQTT ID, grupo, IP fixo/DHCP, MQTT
- Botão físico (GPIO0/D3): pressão 1–3s abre portal, pressão >5s faz reset total
- Watchdog de reconexão WiFi a cada 30s
- Suporte a IP fixo com reaplique automático após reconexão

### OTA

- **ArduinoOTA** — atualização via IDE Arduino ou terminal na rede local
  - Hostname: `hostname_buf` (ex: `irhub8266`)
  - Senha: ChipID em hexadecimal (8 dígitos)
- **OTA via browser** — atualização pelo dashboard em `/system`
  - Endpoint: `POST /update` com autenticação HTTP Basic
  - Aceita arquivo `.bin` gerado pelo Arduino IDE
  - Barra de progresso integrada ao frontend

### Telnet

- Porta 8266
- Comandos: `status`, `led`, `ir`, `ir receptor <modo>`, `irteste [on/off]`, `aht10`, `mqtt`, `network`, `info`, `heap`, `senha`, `reboot`, `help`

---

## ⚙️ Hardware

### Pinos utilizados

| Função         | GPIO    | NodeMCU | Observações                        |
|----------------|---------|---------|------------------------------------|
| Botão Reset    | GPIO0   | D3      | INPUT_PULLUP — afeta boot          |
| LED A          | GPIO2   | D4      | Feedback do sistema (onboard)      |
| LED B          | GPIO5   | D1      | Saída digital controlável          |
| IR Emissor     | GPIO4   | D2      | Recomendado transistor NPN         |
| AHT10 SDA      | GPIO12  | D6      | I2C — sensor AHT10                 |
| AHT10 SCL      | GPIO13  | D7      | I2C — sensor AHT10                 |
| IR Receptor    | GPIO14  | D5      | VS1838B ou equivalente (38kHz)     |

---

## ⚙️ Configuração

A configuração é feita pelo portal WiFiManager (primeira inicialização ou pressão do botão) ou pela página `/settings` do frontend. Os parâmetros são persistidos em `/config.json` no LittleFS.

### Portal WiFi AP

O portal de configuração WiFi (`irhub8266` / `192.168.4.1`) é protegido por `PasswordWS` (mesma senha do WebSocket).

- **Padrão:** ChipID em hex (8 dígitos)
- **Configurável em:** `/settings` → "Senha WebSocket"

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
| Admin User    | `admin`      | Usuário de login do dashboard    |

### Senhas padrão

| Acesso            | Usuário        | Senha padrão              | Configurável em  |
|-------------------|----------------|---------------------------|------------------|
| HTTP Basic Auth   | `admin`        | ChipID em hex (8 dígitos) | `/settings`      |
| WebSocket / Login | `admin`        | ChipID em hex (8 dígitos) | `/settings`      |
| OTA (ArduinoOTA)  | —              | ChipID em hex (8 dígitos) | não              |
| OTA (browser)     | `admin`        | ChipID em hex (8 dígitos) | `/settings`      |
| Portal WiFi AP    | —              | ChipID em hex (8 dígitos) | `/settings`      |

> O usuário padrão é `admin` e pode ser alterado em `/settings` → "Usuário Admin".  
> O ChipID é exibido no monitor serial durante o boot. Após conectar à rede, consulte as senhas via Telnet com o comando `senha`.

---

## 🔐 Autenticação

### HTTP Basic Auth

Protege as rotas de manipulação de arquivos e atualização OTA.

| Rota        | Método | Protegida |
|-------------|--------|-----------|
| `/upload`   | POST   | ✅        |
| `/delete`   | POST   | ✅        |
| `/download` | GET    | ✅        |
| `/update`   | POST   | ✅        |
| `/files`    | GET    | ✅        |

- **Usuário:** `admin` (configurável em `/settings` → "Usuário Admin")
- **Senha:** ChipID em hex por padrão; configurável em `/settings` → "Senha WebSocket"

---

### WebSocket (porta 81)

A conexão WebSocket é aberta sem autenticação. O estado do dispositivo (system, network, MQTT, IR, LED) é transmitido imediatamente ao conectar, permitindo acesso de leitura sem senha.

Comandos que alteram estado ou configuração exigem autenticação prévia:

| Comando           | Requer auth |
|-------------------|-------------|
| `sendIR`          | ❌          |
| `toggleIREmissor` | ❌          |
| `setIRReceptor`   | ❌          |
| `toggleLEDB`      | ❌          |
| `saveConfig`      | ✅          |
| `wifiPortal`      | ✅          |
| `resetWifi`       | ✅          |
| `resetConfig`     | ✅          |
| `reboot`          | ❌          |
| `telnetCmd`       | ❌          |

#### Fluxo de autenticação WebSocket


```
Cliente                        Dispositivo
│                               │
│── { cmd: "getChipId" } ──────►│
│◄─ { type: "chipId",           │
│     value: "AABBCCDD" } ──────│
│                               │
│  hash = SHA-256(senha + chipId)
│                               │
│── { cmd: "auth",              │
│     hash: "<sha256>" } ───────►│
│◄─ { type: "authOk" } ─────────│
│        ou                     │
│◄─ { type: "authError" } ──────│
```

- O hash é calculado como `SHA-256(PasswordWS + ChipID_hex)`
- O ChipID é obtido em tempo real via `getChipId` para evitar replay attacks
- A senha (`PasswordWS`) é configurável em `/settings` → "Senha WebSocket"
- O padrão é o ChipID em hex (8 dígitos), consultável via Telnet: `senha`

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
│   └── ledb/
│       └── state
└── sensor/
    ├── aht10/
    │   ├── state
    │   └── status
    └── ir/
        ├── config/state
        ├── received/state
        └── sent/state
```

Para detalhes completos, consulte a documentação em [`docs/`](docs/):

- [`MQTT.md`](docs/MQTT.md) — payloads e comandos MQTT
- [`pinout.md`](docs/pinout.md) — mapeamento de pinos
- [`hardware.md`](docs/hardware.md) — componentes e circuito
- [`config-remotes.md`](docs/config-remotes.md) — criação de controles remotos

---

## 🔄 Inicialização

Durante o boot o dispositivo executa na ordem:

1. Monta LittleFS e carrega `config.json`
2. Conecta ao WiFi via WiFiManager (abre portal se necessário)
3. Inicia servidor HTTP (porta 80) e WebSocket (porta 81)
4. Configura OTA (ArduinoOTA + HTTP)
5. Configura e conecta ao MQTT (se habilitado)
6. Inicializa IR (emissor GPIO4/D2, receptor GPIO14/D5)
7. Inicializa AHT10 (I2C GPIO12/D6 — GPIO13/D7, somente se habilitado em `config.json`)
8. Pisca LED A 5 vezes (feedback de boot)

---

## 📁 Estrutura de arquivos

### Firmware (Arduino)

```
IRHub-8266/
├── IRHub-8266.ino     — setup/loop
├── globals.h          — declarações externas e enums
├── globals.cpp        — variáveis globais, LED, senhas
├── Network.ino        — WiFi, WiFiManager e watchdog
├── Config.ino         — load/save/reset config
├── ServerWS.ino       — HTTP + WebSocket
├── MQTT.ino           — conexão MQTT, publishers e callback
├── IR.ino             — emissor, receptor, parser, feedback
├── AHT10.ino          — sensor AHT10
├── OTA.ino            — atualização OTA (ArduinoOTA + HTTP)
├── Telnet.ino         — servidor telnet + comandos
└── Debug.ino          — funções de debug
```

### Frontend (LittleFS)

```
data/
├── index.html              — shell SPA (navbar, drawer, app-content)
├── home_content.html       — partial: controle IR + remotes manager
├── ir_content.html         — partial: emissor e receptor IR
├── system_content.html     — partial: status do sistema e console
├── settings_content.html   — partial: configuração do dispositivo
├── app.js                  — lógica compartilhada (WebSocket, SPA, UI)
├── style.css               — estilos globais
└── remotes.json            — modelos de controle remoto
```

---

## 🛠 Dependências

| Biblioteca               | Uso                             |
|--------------------------|---------------------------------|
| ESP8266WiFi              | WiFi                            |
| ESP8266WebServer         | Servidor HTTP                   |
| ESP8266mDNS              | mDNS                            |
| WiFiManager              | Portal de configuração          |
| WebSocketsServer         | WebSocket (porta 81)            |
| PubSubClient             | MQTT                            |
| ArduinoOTA               | Atualização OTA via IDE         |
| ESP8266HTTPUpdateServer  | Atualização OTA via browser     |
| IRremoteESP8266          | Envio e recepção IR             |
| Adafruit_AHTX0           | Sensor AHT10                    |
| ArduinoJson              | Serialização JSON               |
| LittleFS                 | Sistema de arquivos             |

---

## 🚀 Como usar

1. Grave o firmware via Arduino IDE
2. Faça upload dos arquivos da pasta `data/` via LittleFS (Arduino IDE → Tools → ESP8266 LittleFS Data Upload)
3. Na primeira inicialização, conecte-se à rede `irhub8266` com a senha padrão (ChipID em hex, 8 dígitos — visível no monitor serial do Arduino IDE durante o boot) e acesse `192.168.4.1` para configurar WiFi e MQTT
4. Após conectar, acesse `http://irhub8266.local` ou pelo IP atribuído
5. Use a página `/settings` para ajustar configurações sem precisar do portal

> Após qualquer upload de arquivos LittleFS, faça **Ctrl+Shift+R** no browser para limpar o cache.

---

## 🔄 Atualização de Firmware (OTA)

### Via Arduino IDE

1. Com o dispositivo na mesma rede, abra **Tools → Port** e selecione a porta de rede do IRHub-8266 (aparece como `irhub8266 at 192.168.x.x`)
2. Grave normalmente com **Ctrl+U**
3. Quando solicitado, informe a senha OTA (ChipID em hex — consulte com o comando `senha` no Telnet)

### Via browser (dashboard)

1. No Arduino IDE 2, compile o projeto com **Ctrl+R**
2. Exporte o binário com **Sketch → Export Compiled Binary** (ou **Ctrl+Alt+S**)
3. O arquivo `IRHub-8266.ino.bin` será gerado na pasta do projeto
4. Acesse o dashboard em `http://irhub8266.local`, vá para a página **System**
5. Na seção **Atualização de Firmware**, selecione o arquivo `.bin`
6. Clique em **📤 Enviar**, confirme e informe a senha
7. Aguarde o upload — o dispositivo reiniciará automaticamente ao concluir

> Use sempre o arquivo `IRHub-8266.ino.bin` — **não** use o `IRHub-8266.ino.bootloader.bin`, que inclui o bootloader e não é compatível com OTA.
