# Pinout — IRHub-8266

Tabela de mapeamento GPIO → Função para NodeMCU ESP8266.

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

## ⚠️ Observações Importantes

- GPIO0 afeta o boot: nível LOW durante power-on entra em modo de gravação.
- GPIO2 deve estar HIGH durante o boot (LED A apagado = HIGH por padrão no código).
- GPIO16 não suporta interrupções — não utilizado neste projeto.
- I2C (SDA/SCL) e IR utilizam GPIOs distintos — sem conflito de pinos.
