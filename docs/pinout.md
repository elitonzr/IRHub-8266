# Pinout --- IRHub-8266

Tabela de mapeamento GPIO → Função.

  Função        GPIO      Observações
  ------------- --------- ------------------------
  LED           GPIO2     Geralmente LED onboard
  IR Emissor    GPIO4     Pode usar transistor
  IR Receptor   GPIO14    Suporte a interrupção
  I2C SDA       GPIO4\*   Caso use AHT10
  I2C SCL       GPIO5     Caso use AHT10

\* Ajustar conforme necessidade do projeto.

------------------------------------------------------------------------

## ⚠ Observações Importantes

-   GPIO16 não suporta interrupções.
-   Alguns GPIOs afetam o boot (GPIO0, GPIO2, GPIO15).
-   Evitar conflitos entre IR e I2C se compartilharem pinos.
