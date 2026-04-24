// ==========================
// TENTATIVA DE CONEXÃO DIRETA
// Retorna true se conectou, false se falhou.
// ==========================
bool tryWiFiConnect() {
  if (strlen(wifi_ssid_buf) == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname_buf);

  IPAddress ip, gw, sn, dns(8, 8, 8, 8);
  if (strlen(ipStr) > 6 && ip.fromString(ipStr) && gw.fromString(gwStr) && sn.fromString(snStr)) {
    WiFi.config(ip, gw, sn, dns);
    Serial.println("[WiFi]    - IP fixo aplicado");
  }

  WiFi.begin(wifi_ssid_buf, wifi_password_buf);
  Serial.print("[WiFi]    - Conectando a ");
  Serial.println(wifi_ssid_buf);

  setLedMode(LED_WIFI_CONNECTING);

  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t > 10000) {
      Serial.println("[WiFi]    - Timeout.");
      return false;
    }
    handleFeedbackLED();
    delay(100);
  }
  return true;
}

// ==========================
// SETUP WIFI PRINCIPAL
// ==========================
void setup_WiFi() {
  setLedMode(LED_WIFI_DISCONNECTED);
  loadConfig();

  if (tryWiFiConnect()) {
    Serial.println("[WiFi]    - Conectado!");

    if (MDNS.begin(hostname_buf)) {
      MDNS.addService("http", "tcp", 80);
      Serial.println(String("[mDNS] http://") + hostname_buf + ".local");
    }

    setLedMode(LED_IDLE);
    recalcularTopicos();
    return;
  }

  // Sem credenciais ou falha → abre portal próprio
  startConfigPortal();
}

// ==========================
// PORTAL DE CONFIGURAÇÃO PRÓPRIO
// ==========================
void startConfigPortal() {
  setLedMode(LED_WIFI_DISCONNECTED);
  debugLogPrint("[WiFi]", "Iniciando portal de configuração próprio");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(hostname_buf, PasswordPortal);
  delay(500);

  IPAddress apIP(192, 168, 4, 1);
  IPAddress apGW(192, 168, 4, 1);
  IPAddress apSN(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apGW, apSN);

  // DNS Server — redireciona qualquer domínio para o portal (captive portal)
  DNSServer dnsServer;
  dnsServer.start(53, "*", apIP);

  printPortalCredentials();

  // Página HTML do portal
  server.on("/start", HTTP_GET, []() {
    // Faz scan das redes disponíveis
    int n = WiFi.scanNetworks();
    String options = "";
    for (int i = 0; i < n; i++) {
      options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + " dBm)</option>";
    }

    String html = R"rawliteral(
  <!DOCTYPE html><html><head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width,initial-scale=1'>
  <title>IRHub-8266 Setup</title>
  <link rel='stylesheet' href='/style.css'>
  </head><body>
  <nav class='navbar'><span class='navbar-brand'>IRHub-8266 Setup</span></nav>
  <div class='page-content'>
  <section class='card'>
    <header>Configuração WiFi</header>
    <form action='/save' method='POST'>
      <div class='item'>SSID:
        <select name='ssid' style='width:100%'>)rawliteral";
    html += options;
    html += R"rawliteral(</select>
      </div>
      <div class='item'>Senha WiFi:
        <input type='password' name='wifi_pass' placeholder='Senha da rede'>
      </div>
      <hr>
      <div class='item'>Hostname:
        <input type='text' name='hostname' value=')rawliteral";
    html += hostname_buf;
    html += R"rawliteral('>
      </div>
      <div class='item'>
        <button type='submit' class='btn-primary btn-block'>💾 Salvar e Reiniciar</button>
      </div>
    </form>
  </section>
  </div>
  </body></html>
  )rawliteral";
    server.send(200, "text/html", html);
  });

  // Captive portal redirect
  server.onNotFound([&apIP]() {
    server.sendHeader("Location", String("http://") + apIP.toString(), true);
    server.send(302, "text/plain", "");
  });

  // Recebe o form e salva
  server.on("/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String wifiPass = server.arg("wifi_pass");
    String hostname = server.arg("hostname");

    if (ssid.length() == 0) {
      server.send(400, "text/plain", "SSID obrigatório");
      return;
    }

    strlcpy(wifi_ssid_buf, ssid.c_str(), sizeof(wifi_ssid_buf));
    strlcpy(wifi_password_buf, wifiPass.c_str(), sizeof(wifi_password_buf));
    if (hostname.length() > 0)
      strlcpy(hostname_buf, hostname.c_str(), sizeof(hostname_buf));

    recalcularTopicos();
    saveConfig();

    server.send(200, "text/html",
                "<html><body style='font-family:sans-serif;background:#111827;color:#f9fafb;"
                "display:flex;align-items:center;justify-content:center;height:100vh'>"
                "<h2>✅ Salvo! Reiniciando...</h2></body></html>");
    delay(1500);
    ESP.restart();
  });

  server.begin();

  // Loop do portal — aguarda configuração ou timeout de 3 minutos
  unsigned long tPortal = millis();
  while (millis() - tPortal < 180000) {
    dnsServer.processNextRequest();
    server.handleClient();
    handleFeedbackLED();
    yield();
  }

  // Timeout — reinicia sem salvar
  debugLogPrint("[WiFi]", "Portal timeout. Reiniciando...");
  ESP.restart();
}

// ==========================
// RECALCULA TÓPICOS MQTT
// ==========================
void recalcularTopicos() {
  myTopic = String(mqtt_id_buf) + "-" + String(grupo_buf);
  clientID = String(myTopic) + "-" + String(ESP.getChipId());
}

void printPortalCredentials() {
  debugLogPrintf("[AUTH]", "%-6s | SSID    : %-12s | Senha: %-10s | %s", "Portal", hostname_buf, PasswordPortal, "http://192.168.4.1/start");
}