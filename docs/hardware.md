# Hardware — IRHub-8266

Este documento descreve os componentes físicos utilizados no projeto IRHub-8266.

---

## 🧠 Microcontrolador

**ESP8266 (NodeMCU ou compatível)**

- WiFi integrado 2.4GHz
- Clock: 80/160 MHz
- GPIO com suporte a interrupções (exceto GPIO16)
- Flash típica de 4MB

---

## 📡 Módulo IR

### IR Emissor — GPIO4 (D2)

- LED infravermelho 940nm
- Recomendado uso com resistor (~100–220Ω)
- Opcional: transistor NPN para maior alcance

### IR Receptor — GPIO14 (D5)

- Módulo tipo VS1838B ou equivalente
- Opera em 38kHz
- Alimentação 3.3V

---

## 🌡 Sensor AHT10 (Opcional)

Sensor digital de temperatura e umidade.

- Interface: I2C — SDA: GPIO12 (D6), SCL: GPIO13 (D7)
- Alimentação: 3.3V
- Endereço padrão: 0x38
- Habilitação configurável em runtime via `/settings`

---

## 💡 LEDs de Status

### LED A — GPIO2 (D4)

LED de feedback do sistema (geralmente LED onboard do NodeMCU). Indica:

- Boot: 5 piscadas rápidas
- Recepção IR: 1 piscada
- Envio IR: 1 piscada
- Modos contínuos: WiFi connecting, WiFi desconectado, MQTT desconectado, OTA, erro de FS

### LED B — GPIO5 (D1)

Saída digital controlável pelo usuário via WebSocket, MQTT ou Telnet.

---

## 🔘 Botão Reset — GPIO0 (D3)

- Pressão 1–3s: abre portal WiFiManager
- Pressão >5s: reset total (apaga config.json e credenciais WiFi)

---

## 🔌 Alimentação

- 5V via USB
- Regulador onboard converte para 3.3V

---

## 📶 Requisitos de Rede

- Broker MQTT acessível na rede local (opcional)
- WiFi 2.4GHz
