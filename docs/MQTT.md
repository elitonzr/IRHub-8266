# 📡 IRHub-8266 — Guia MQTT

Este documento descreve a comunicação MQTT do projeto **IRHub-8266**, com base na implementação atual de `MQTT.ino` e `Callback.ino`.

---

## 🧠 Conceito Geral

O IRHub-8266 usa MQTT para comunicação bidirecional:

- 📥 **Subscription:** recebe comandos
- 📤 **Publishers:** envia estados, sensores e feedbacks

O tópico base (`myTopic`) é formado por:

```
<mqtt_id>-<grupo>
```

Exemplo com os valores padrão (`mqtt_id = IRHub-8266`, `grupo = Sala`):

```
IRHub-8266-Sala
```

---

## 📥 Subscription

### `IRHub-8266-Sala/command`

Único tópico de entrada. Todos os comandos são enviados aqui como JSON.

```json
{ "cmd": "<comando>", ... }
```

---

## 🔧 Comandos disponíveis

| `cmd`         | Descrição                              |
|---------------|----------------------------------------|
| `info`        | Solicita publicação de informações     |
| `ledb`        | Controla o LED B                       |
| `ir_send`     | Envia código IR                        |
| `ir_receptor` | Define modo do receptor IR             |
| `ir_test`     | Ativa/desativa modo de teste do emissor|
| `reboot`      | Reinicia o dispositivo                 |
| `wifi_reset`  | Reseta WiFi e reinicia                 |
| `config`      | Altera configurações                   |

---

### 💡 LED

```json
{ "cmd": "ledb", "action": "on" }
```

Valores de `action`: `on`, `off`, `toggle`

---

### 📡 Envio IR

```json
{
  "cmd": "ir_send",
  "protocol": "NEC",
  "code": "0x20DF10EF",
  "bits": 32
}
```

- `code`: aceita hex (`0x20DF10EF`) ou decimal (`551489775`)
- `bits`: padrão `32` se omitido
- `protocol`: veja protocolos suportados abaixo

**Protocolos suportados:** `NEC`, `SONY`, `RC5`, `RC6`, `SAMSUNG`, `NIKAI`, `LG`, `JVC`, `WHYNTER`

---

### 📡 Receptor IR

```json
{
  "cmd": "ir_receptor",
  "mode": "KNOWN"
}
```

Valores de `mode`:

| Valor      | Comportamento                                    |
|------------|--------------------------------------------------|
| `ALL`      | Aceita qualquer protocolo                        |
| `KNOWN`    | Aceita qualquer protocolo conhecido (não-UNKNOWN)|
| `DISABLED` | Recepção desabilitada                            |
| `NEC`      | Apenas NEC                                       |
| `SONY`     | Apenas SONY                                      |
| `RC5`      | Apenas RC5                                       |
| `RC6`      | Apenas RC6                                       |
| `SAMSUNG`  | Apenas SAMSUNG                                   |
| `NIKAI`    | Apenas NIKAI                                     |
| `LG`       | Apenas LG                                        |
| `JVC`      | Apenas JVC                                       |
| `WHYNTER`  | Apenas WHYNTER                                   |

> O modo selecionado é persistido em `config.json` e sobrevive a reboots.

---

### 🔴 Teste IR

Ativa o ciclo de desligamento universal (envia sequencialmente códigos de vários fabricantes a cada 2s).

```json
{ "cmd": "ir_test", "enabled": true }
```

---

### 📋 Info

Solicita publicação imediata de informações.

```json
{ "cmd": "info", "type": "all" }
```

Valores de `type`:

| Valor    | Publica em               |
|----------|--------------------------|
| `all`    | device, network, mqtt, status |
| `uptime` | `info/uptime`            |
| `ir`     | `sensor/ir/config/state` |
| `aht10`  | `sensor/aht10/state` (somente se habilitado) |
| `led`    | `switch/ledb/state`       |

---

### 🔄 Reboot

```json
{ "cmd": "reboot" }
```

---

### 📶 Reset WiFi

Limpa credenciais WiFi e reinicia.

```json
{ "cmd": "wifi_reset" }
```

---

### ⚙️ Config

Altera configurações em runtime. Atualmente suporta `hostname`.

```json
{ "cmd": "config", "hostname": "IRHub-Sala" }
```

---

## 📤 Publishers

### `IRHub-8266-Sala/status`

Birth/Will — publicado com `retain = true`.

```json
{ "state": "online" }
```

> Em caso de desconexão inesperada, o broker publica automaticamente `{ "state": "offline" }` via Last Will.

---

### `IRHub-8266-Sala/info/device`

Publicado na conexão e sob demanda (`info all`).

```json
{
  "build_datetime": "Apr 10 2026 12:00:00",
  "version": "0.5.2",
  "chip_id": 12345678,
  "hostname": "irhub8266",
  "mqtt_id": "IRHub-8266",
  "grupo": "Sala",
  "topic_main": "IRHub-8266-Sala",
  "client_id": "IRHub-8266-Sala-12345678",
  "aht10_enabled": true
}
```

---

### `IRHub-8266-Sala/info/network`

Publicado na conexão, sob demanda e a cada 5 minutos.

```json
{
  "wifi": "MinhaRede",
  "ip": "192.168.1.10",
  "gateway": "192.168.1.1",
  "mask": "255.255.255.0",
  "rssi": -60
}
```

---

### `IRHub-8266-Sala/info/mqtt`

Publicado na conexão e sob demanda.

```json
{
  "server": "mqtt.local",
  "client_id": "IRHub-8266-Sala-12345678",
  "topic_main": "IRHub-8266-Sala/#",
  "connect": 10,
  "erro": 1
}
```

---

### `IRHub-8266-Sala/info/uptime`

Publicado sob demanda e a cada 5 minutos.

```json
{
  "uptime_formatted": "0d 00h 10m 32s",
  "uptime_seconds": 632
}
```

---

### `IRHub-8266-Sala/switch/ledb/state`

Publicado sempre que o estado do LED muda.

```json
{ "state": "ON" }
```

Valores: `ON`, `OFF`

---

### `IRHub-8266-Sala/sensor/aht10/state`

Publicado a cada 5 minutos quando o sensor está online e habilitado.

> Não publicado quando `aht10_enabled = false`.

```json
{
  "temperature": 25.3,
  "humidity": 60.2
}
```

---

### `IRHub-8266-Sala/sensor/aht10/status`

Publicado quando o sensor está offline ou em erro (somente quando habilitado).

> Não publicado quando `aht10_enabled = false`.

```json
{ "state": "offline" }
```

Valores de `state`: `online`, `offline`, `error`

---

### `IRHub-8266-Sala/sensor/ir/config/state`

Publicado ao alterar modo do receptor ou emissor.

```json
{
  "sender": { "cmd_test": false },
  "received": { "protocol": "KNOWN" }
}
```

---

### `IRHub-8266-Sala/sensor/ir/received/state`

Publicado a cada sinal IR recebido e aceito pelo receptor.

```json
{
  "protocol": "NEC",
  "decode_type": 3,
  "bits": 32,
  "dec": "551489775",
  "hex": "0x20DF10EF"
}
```

---

### `IRHub-8266-Sala/sensor/ir/sent/state`

Publicado após cada tentativa de envio IR.

```json
{
  "type": "ir_emissor",
  "emissor_teste": false,
  "status": "ok",
  "origem": "[MQTT]",
  "protocol": "NEC",
  "bits": 32,
  "dec": 551489775,
  "hex": "0x20DF10EF",
  "millis": 123456
}
```

Valores de `status`: `ok`, `fail`, `code vazio`, `protocol desconhecido`, `código inválido`, `JSON inválido`

---

## 🔄 Fluxo de funcionamento

```
Cliente  →  command (JSON)
               ↓
         processaComando()
               ↓
         executa ação
               ↓
         publica feedback em state/info
```

---

## ✅ Boas práticas

- Use sempre JSON válido
- Monitore `status` com LWT para detectar quedas
- Use `retain = true` ao se inscrever em `status`
- Prefira `info/type` específico ao invés de `all` quando possível

---

## 🗺️ Mapa completo de tópicos

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
