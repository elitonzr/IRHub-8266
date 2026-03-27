# MQTT Topics — IRHub-8266

Este documento descreve todos os tópicos MQTT utilizados pelo **IRHub-8266**, incluindo direção, payload e observações.

## Base Topic
```
<myTopic> = IRHub-8266-<nome>
```

Exemplo:
```
IRHub-8266-Sala
```

---

## 📡 Status & Informação

### `<myTopic>/info/status`
- **Direção:** publish
- **Retain:** true
- **Payload:** `online` | `offline`
- **Descrição:** Birth / Last Will do dispositivo

### `<myTopic>/info/uptime`
- **Direção:** publish
- **Payload:** segundos desde o boot

### `<myTopic>/info/software`
- **Direção:** publish
- **Payload (JSON):**
```json
{
  "firmware": "v2025",
  "version": "gcc",
  "data": "Feb 15 2026",
  "hora": "21:30:00",
  "file": "IRHub-8266.ino",
  "mqtt_client_id": "IRHub-8266-Sala_123456"
}
```

### `<myTopic>/info/network`
- **Direção:** publish
- **Payload (JSON):**
```json
{
  "Wifi": "SSID",
  "IP": "192.168.1.50",
  "Gateway": "192.168.1.1",
  "Mask": "255.255.255.0",
  "RSSI": -62
}
```

### `<myTopic>/info/mqtt`
- **Direção:** publish
- **Payload (JSON):**
```json
{
  "connect": 10,
  "erro": 2
}
```

---

## 🎛️ IR — Envio e Recepção

### `<myTopic>/IR/typeSendCod`
- **Direção:** publish
- **Payload:** inteiro (`0–3`)
- **Descrição:** modo atual de envio IR

### `<myTopic>/sensores/IR/NEC`
- **Direção:** publish
- **Payload (JSON):**
```json
{
  "DEC": 551489412,
  "HEX": "20DF10EF"
}
```

### `<myTopic>/sensores/IR/24bits`
- **Direção:** publish
- **Payload:** JSON semelhante ao NEC

### `<myTopic>/sensores/IR/Desconhecido`
- **Direção:** publish
- **Payload:** JSON semelhante

---

## 🌡️ Sensores AHT10

### `<myTopic>/sensores/temperatura`
- **Direção:** publish
- **Payload:** string float (`23.4`)

### `<myTopic>/sensores/umidade`
- **Direção:** publish
- **Payload:** string float (`56.1`)

---

## 🚦 Outputs

### `<myTopic>/info/Outputs`
- **Direção:** publish
- **Payload (JSON):**
```json
{
  "value": [1]
}
```

---

## 📥 Comandos (subscribe)

### `<myTopic>/command/#`
- **Direção:** subscribe
- **Descrição:** comandos gerais (LED, IR, etc)

---

## Observações Técnicas
- Payloads são montados com `snprintf` e buffers fixos
- Tópicos críticos usam **retain**
- Reconexão MQTT é não bloqueante


já corrigi todos os bugs.
Agora crie um arquivo .md com o funcionamento de cada tópico do MQTT para que o usuário entenda com o dispositivo funciona, o arquivo deve ter exemplos de uso para melhor entendimento.


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