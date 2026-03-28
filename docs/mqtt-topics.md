```txt
  Subscriptions:
    IRHub-8266-Sala/command
    IRHub-8266-Sala/switch/led/command
    IRHub-8266-Sala/sensor/ir/send/command
    IRHub-8266-Sala/sensor/ir/receptor/command
  Publishers:
    IRHub-8266-Sala/status
    IRHub-8266-Sala/info/device
    IRHub-8266-Sala/info/network
    IRHub-8266-Sala/info/mqtt
    IRHub-8266-Sala/info/uptime
    IRHub-8266-Sala/switch/led/state
    IRHub-8266-Sala/sensor/aht10/state
    IRHub-8266-Sala/sensor/aht10/status
    IRHub-8266-Sala/sensor/ir/config/state
    IRHub-8266-Sala/sensor/ir/received/state
    IRHub-8266-Sala/sensor/ir/sent/state
```
# 📡 IRHub-8266 - Guia MQTT

Este documento descreve como funciona a comunicação MQTT do projeto **IRHub-8266**, com base na implementação real do arquivo `MQTT.ino`.

---

## 🧠 Conceito Geral

O IRHub-8266 utiliza MQTT para comunicação bidirecional:

* 📥 **Subscriptions:** recebe comandos
* 📤 **Publishers:** envia estados, sensores e feedbacks

O tópico base é definido por:

```
<myTopic>/#
```

Exemplo:

```
IRHub-8266-Sala/#
```

---

# 📥 Subscriptions (Entrada)

## 🔹 `IRHub-8266-Sala/command` ⭐ (PRINCIPAL)

📌 **Novo padrão oficial (JSON)**

Todos os comandos modernos utilizam esse tópico.

### 🧾 Estrutura padrão

```json
{ "cmd": "<comando>", ... }
```

---

## 🔧 Comandos disponíveis

| cmd           | Descrição               |
| ------------- | ----------------------- |
| `info`        | Solicita informações    |
| `led`         | Controla LED            |
| `ir_send`     | Envia IR                |
| `ir_receptor` | Define modo do receptor |
| `ir_test`     | Ativa/desativa teste IR |
| `reboot`      | Reinicia dispositivo    |
| `wifi_reset`  | Reseta WiFi             |
| `config`      | Altera configurações    |

---

## 💡 LED

```json
{ "cmd": "led", "action": "on" }
```

Valores:

* on
* off
* toggle

---

## 📡 Envio IR

```json
{
  "cmd": "ir_send",
  "protocol": "NEC",
  "code": "0x20DF10EF",
  "bits": 32
}
```

✔ Aceita código em decimal ou hexadecimal
✔ Reutiliza o parser interno (`processaIRJson`)

---

## 📡 Receptor IR

```json
{
  "cmd": "ir_receptor",
  "mode": "NEC"
}
```

Valores:

* DESABILITADO
* NEC
* NIKAI
* NEC e NIKAI
* TUDO

---

## 🔴 Teste IR

```json
{ "cmd": "ir_test", "enabled": true }
```

---

## 📡 Info

```json
{ "cmd": "info", "type": "all" }
```

Tipos:

* all
* uptime
* ir
* aht10
* led

---

## 🔄 Reboot

```json
{ "cmd": "reboot" }
```

---

## 📶 Reset WiFi

```json
{ "cmd": "wifi_reset" }
```

---

## ⚙️ Config

```json
{ "cmd": "config", "hostname": "IRHub-Sala" }
```

---

# ⚠️ Tópicos LEGADOS (compatibilidade)

Ainda existentes no firmware, mas recomendados apenas para compatibilidade:

* `switch/led/command`
* `sensor/ir/send/command`
* `sensor/ir/receptor/command`

👉 Use preferencialmente `command`

---

# 📤 Publishers (Saída)

## 🔹 `status`

```json
{ "state": "online" }
```

✔ Usa **Last Will** → publica `offline` automaticamente

---

## 🔹 `info/device`

```json
{
  "version": "1.0",
  "hostname": "IRHub-8266-Sala"
}
```

Inclui:

* build
* chip_id
* grupo
* topic

---

## 🔹 `info/network`

```json
{
  "wifi": "MinhaRede",
  "ip": "192.168.1.10",
  "rssi": -60
}
```

---

## 🔹 `info/mqtt`

```json
{
  "server": "192.168.1.2",
  "connect": 10,
  "erro": 1
}
```

---

## 🔹 `info/uptime`

```json
{
  "uptime_formatted": "00:10:32",
  "uptime_seconds": 632
}
```

---

## 🔹 `switch/led/state`

```json
{ "state": "ON" }
```

---

## 🔹 `sensor/aht10/state`

```json
{ "temperature": 25.3, "humidity": 60.2 }
```

---

## 🔹 `sensor/aht10/status`

```json
{ "state": "online" }
```

---

## 🔹 `sensor/ir/config/state`

```json
{
  "sender": { "cmd_test": true },
  "received": { "protocol": "NEC" }
}
```

---

## 🔹 `sensor/ir/received/state`

```json
{
  "protocol": "NEC",
  "bits": 32,
  "dec": 123456,
  "hex": "0x20DF10EF"
}
```

---

## 🔹 `sensor/ir/sent/state`

```json
{
  "status": "ok",
  "protocol": "NEC"
}
```

---

# 🔄 Fluxo de funcionamento

1. Cliente publica JSON em `command`
2. IRHub processa via `processaComando()`
3. Executa ação
4. Publica feedback nos tópicos `state` ou `info`

---

# ✅ Boas práticas

* Use sempre JSON
* Evite tópicos legados
* Monitore `status` (LWT)
* Use retain para status

---

# 🚀 Conclusão

O IRHub-8266 agora implementa uma **API MQTT moderna, padronizada e extensível**, ideal para integração com:

* Home Assistant
* Node-RED
* Scripts MQTT

---

