/************ WIFI ************/
void setup_wifi() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("         Configurando WiFi         ");
  Serial.println("=================================");

  delay(10);

  // Configura o IP, gateway e subnetMask
  WiFi.config(ip, gateway, subnet);
  // Conecta à rede
  WiFi.begin(wifi_ssid, wifi_password);

  // Exibe informações sobre a configuração de conexão
  Serial.print("    Conectando à          : ");
  Serial.println(wifi_ssid);
  Serial.print("    Endereço IP           : ");
  Serial.println(WiFi.localIP());
  Serial.print("    Gateway               : ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("    Máscara de sub-rede   : ");
  Serial.println(WiFi.subnetMask());

  // Aguarda conexão
  Serial.println();
  Serial.print("Tentando conexão WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Exibe o endereços após conectar
  Serial.println();
  Serial.println();
  Serial.print("    Conectado à          : ");
  Serial.println(wifi_ssid);
  Serial.print("    Endereço IP          : ");
  Serial.println(WiFi.localIP());
  Serial.print("    Gateway              : ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("    Máscara de sub-rede  : ");
  Serial.println(WiFi.subnetMask());
  Serial.print("    RRSI                 : ");
  Serial.println(WiFi.RSSI());
}