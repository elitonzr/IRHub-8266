# Hardware --- IRHub-8266

Este documento descreve os componentes físicos utilizados no projeto
IRHub-8266.

------------------------------------------------------------------------

## 🧠 Microcontrolador

**ESP8266 (NodeMCU ou compatível)**

Características relevantes: - WiFi integrado 2.4GHz - 80/160 MHz - GPIO
com suporte a interrupções (exceto GPIO16) - Flash típica de 4MB

------------------------------------------------------------------------

## 📡 Módulo IR

### IR Emissor

-   LED infravermelho 940nm
-   Recomendado uso com resistor (\~100--220Ω)
-   Opcional: transistor NPN para maior alcance

### IR Receptor

-   Módulo tipo VS1838B ou equivalente
-   Opera em 38kHz
-   Alimentação 3.3V

------------------------------------------------------------------------

## 🌡 Sensor AHT10 (Opcional)

Sensor digital de temperatura e umidade.

-   Interface: I2C
-   Alimentação: 3.3V
-   Endereço padrão: 0x38

------------------------------------------------------------------------

## 💡 LED de Status

Utilizado para: - Feedback de boot - Indicação de conexão - Testes de
saída digital

------------------------------------------------------------------------

## 🔌 Alimentação

-   5V via USB
-   Regulador onboard converte para 3.3V

------------------------------------------------------------------------

## 📶 Requisitos de Rede

-   Broker MQTT acessível na rede local
-   WiFi 2.4GHz
