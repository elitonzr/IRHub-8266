bool shouldSaveConfig = false;

// ==========================
// HELPERS
// ==========================
bool getParamValue(WiFiManager& wm, WiFiManagerParameter* param, const char* name, char* out, size_t size) {

  if (wm.server && wm.server->hasArg(name)) {
    String val = wm.server->arg(name);
    if (val.length() > 0) {
      strlcpy(out, val.c_str(), size);
      return true;
    }
  }

  if (param) {
    const char* val = param->getValue();
    if (val && strlen(val) > 0) {
      strlcpy(out, val, size);
      return true;
    }
  }

  return false;
}

void copyParam(WiFiManager& wm, WiFiManagerParameter* param, const char* name, char* dest, size_t size) {
  getParamValue(wm, param, name, dest, size);
}

// ==========================
// SETUP WIFI MANAGER
// ==========================
void setup_WiFiManager() {

  setLedMode(LED_WIFI_DISCONNECTED);
  loadConfig();

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.setWebServerCallback([]() {
    ArduinoOTA.handle();
    handleTelnet();
  });

  WiFiManagerParameter *p_hostname, *p_mqtt_id, *p_grupo;
  WiFiManagerParameter *p_ip, *p_gw, *p_sn;
  WiFiManagerParameter *p_mqtt_server, *p_mqtt_port, *p_mqtt_user, *p_mqtt_password, *p_mqtt_enabled;

  setupWiFiManagerParams(
    wifiManager,
    p_hostname, p_mqtt_id, p_grupo,
    p_ip, p_gw, p_sn,
    p_mqtt_server, p_mqtt_port, p_mqtt_user, p_mqtt_password, p_mqtt_enabled);

  IPAddress ip, gw, sn, dns(8, 8, 8, 8);

  if (strlen(ipStr) > 6 && ip.fromString(ipStr) && gw.fromString(gwStr) && sn.fromString(snStr)) {
    WiFi.config(ip, gw, sn, dns);
    Serial.println("[WiFi]    - IP fixo aplicado (pré-conn)");
  }

  WiFi.hostname(hostname_buf);
  setLedMode(LED_WIFI_CONNECTING);

  if (!wifiManager.autoConnect(hostname_buf, PasswordPortal)) {
    Serial.println("[WiFi]    - Falha. Reiniciando...");
    delay(1000);
    ESP.restart();
  }

  Serial.println("[WiFi]    - Conectado!");

  if (WiFi.status() == WL_CONNECTED && MDNS.begin(hostname_buf)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println(String("[mDNS] http://") + hostname_buf + ".local");
  }

  atualizaConfig(
    wifiManager,
    *p_hostname, *p_mqtt_id, *p_grupo,
    *p_ip, *p_gw, *p_sn,
    *p_mqtt_server, *p_mqtt_port, *p_mqtt_user, *p_mqtt_password, *p_mqtt_enabled);

  delete p_hostname;
  delete p_mqtt_id;
  delete p_grupo;
  delete p_ip;
  delete p_gw;
  delete p_sn;
  delete p_mqtt_server;
  delete p_mqtt_port;
  delete p_mqtt_user;
  delete p_mqtt_password;
  delete p_mqtt_enabled;

  Serial.println("SSID: " + WiFi.SSID());
  Serial.println("IP: " + WiFi.localIP().toString());
  Serial.println("Gateway: " + WiFi.gatewayIP().toString());
  Serial.println("Subnet: " + WiFi.subnetMask().toString());
  Serial.println("RSSI: " + String(WiFi.RSSI()));
}

// ==========================
// PORTAL MANUAL
// ==========================
void startWiFiManagerPortal() {

  setLedMode(LED_WIFI_DISCONNECTED);
  debugLogPrint("[WiFi]", "Portal de configuração iniciado");

  WiFi.disconnect(true);
  delay(300);
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(hostname_buf, PasswordPortal);
  delay(500);

  IPAddress apIP = WiFi.softAPIP();

  printPortalCredentials();
  debugLogPrintf("[WiFi]", "http://%s", apIP.toString().c_str());

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setCaptivePortalEnable(true);

  wifiManager.setWebServerCallback([]() {
    ArduinoOTA.handle();
    handleTelnet();
  });

  WiFiManagerParameter *p_hostname, *p_mqtt_id, *p_grupo;
  WiFiManagerParameter *p_ip, *p_gw, *p_sn;
  WiFiManagerParameter *p_mqtt_server, *p_mqtt_port, *p_mqtt_user, *p_mqtt_password, *p_mqtt_enabled;

  setupWiFiManagerParams(
    wifiManager,
    p_hostname, p_mqtt_id, p_grupo,
    p_ip, p_gw, p_sn,
    p_mqtt_server, p_mqtt_port, p_mqtt_user, p_mqtt_password, p_mqtt_enabled);

  if (!wifiManager.startConfigPortal(hostname_buf, PasswordPortal)) {
    delete p_hostname;
    delete p_mqtt_id;
    delete p_grupo;
    delete p_ip;
    delete p_gw;
    delete p_sn;
    delete p_mqtt_server;
    delete p_mqtt_port;
    delete p_mqtt_user;
    delete p_mqtt_password;
    delete p_mqtt_enabled;
    return;
  }

  atualizaConfig(
    wifiManager,
    *p_hostname, *p_mqtt_id, *p_grupo,
    *p_ip, *p_gw, *p_sn,
    *p_mqtt_server, *p_mqtt_port, *p_mqtt_user, *p_mqtt_password, *p_mqtt_enabled);
}

// ==========================
// PARAMETROS
// ==========================
void setupWiFiManagerParams(
  WiFiManager& wifiManager,
  WiFiManagerParameter*& p_hostname,
  WiFiManagerParameter*& p_mqtt_id,
  WiFiManagerParameter*& p_grupo,
  WiFiManagerParameter*& p_ip,
  WiFiManagerParameter*& p_gw,
  WiFiManagerParameter*& p_sn,
  WiFiManagerParameter*& p_mqtt_server,
  WiFiManagerParameter*& p_mqtt_port,
  WiFiManagerParameter*& p_mqtt_user,
  WiFiManagerParameter*& p_mqtt_password,
  WiFiManagerParameter*& p_mqtt_enabled) {
  static char mqtt_port_str[6];
  snprintf(mqtt_port_str, sizeof(mqtt_port_str), "%d", mqtt_port);

  wifiManager.addParameter(new WiFiManagerParameter("<hr><b>Identificação</b>"));

  p_hostname = new WiFiManagerParameter("hostname", "Hostname", hostname_buf, 32);
  p_mqtt_id = new WiFiManagerParameter("mqtt_id", "MQTT ID", mqtt_id_buf, 32);
  p_grupo = new WiFiManagerParameter("grupo", "Grupo", grupo_buf, 32);

  wifiManager.addParameter(p_hostname);
  wifiManager.addParameter(p_mqtt_id);
  wifiManager.addParameter(p_grupo);

  wifiManager.addParameter(new WiFiManagerParameter("<hr><b>Rede</b>"));

  p_ip = new WiFiManagerParameter("ip", "IP", ipStr, 16);
  p_gw = new WiFiManagerParameter("gw", "Gateway", gwStr, 16);
  p_sn = new WiFiManagerParameter("sn", "Subnet", snStr, 16);

  wifiManager.addParameter(p_ip);
  wifiManager.addParameter(p_gw);
  wifiManager.addParameter(p_sn);

  wifiManager.addParameter(new WiFiManagerParameter("<hr><b>MQTT</b>"));

  p_mqtt_server = new WiFiManagerParameter("mqtt_server", "Servidor", mqtt_server, 64);
  p_mqtt_port = new WiFiManagerParameter("mqtt_port", "Porta", mqtt_port_str, sizeof(mqtt_port_str));
  p_mqtt_user = new WiFiManagerParameter("mqtt_user", "Usuário", mqtt_user_buf, 64);
  p_mqtt_password = new WiFiManagerParameter("mqtt_password", "Senha", mqtt_password_buf, 64, "type='password'");

  static char html[256];
  const char* selYes = mqtt_enabled ? "selected" : "";
  const char* selNo = mqtt_enabled ? "" : "selected";

  snprintf(html, sizeof(html),
           "<br><label>MQTT</label><select name='mqtt_enabled' style='width:100%%;'>"
           "<option value='1' %s>Habilitado</option>"
           "<option value='0' %s>Desabilitado</option>"
           "</select>",
           selYes, selNo);


  p_mqtt_enabled = new WiFiManagerParameter(html);

  wifiManager.addParameter(p_mqtt_server);
  wifiManager.addParameter(p_mqtt_port);
  wifiManager.addParameter(p_mqtt_user);
  wifiManager.addParameter(p_mqtt_password);
  wifiManager.addParameter(p_mqtt_enabled);
}

// ==========================
// ATUALIZA CONFIG
// ==========================
void atualizaConfig(
  WiFiManager& wm,
  WiFiManagerParameter& p_hostname,
  WiFiManagerParameter& p_mqtt_id,
  WiFiManagerParameter& p_grupo,
  WiFiManagerParameter& p_ip,
  WiFiManagerParameter& p_gw,
  WiFiManagerParameter& p_sn,
  WiFiManagerParameter& p_mqtt_server,
  WiFiManagerParameter& p_mqtt_port,
  WiFiManagerParameter& p_mqtt_user,
  WiFiManagerParameter& p_mqtt_password,
  WiFiManagerParameter& p_mqtt_enabled) {
  if (!shouldSaveConfig) return;
  shouldSaveConfig = false;

  copyParam(wm, &p_hostname, "hostname", hostname_buf, sizeof(hostname_buf));
  copyParam(wm, &p_mqtt_id, "mqtt_id", mqtt_id_buf, sizeof(mqtt_id_buf));
  copyParam(wm, &p_grupo, "grupo", grupo_buf, sizeof(grupo_buf));

  char tmpIp[16] = "";
  char tmpGw[16] = "";
  char tmpSn[16] = "";

  copyParam(wm, &p_ip, "ip", tmpIp, sizeof(tmpIp));
  copyParam(wm, &p_gw, "gw", tmpGw, sizeof(tmpGw));
  copyParam(wm, &p_sn, "sn", tmpSn, sizeof(tmpSn));

  IPAddress testIp, testGw, testSn;

  bool dhcp = (strlen(tmpIp) == 0 || strcmp(tmpIp, "0.0.0.0") == 0);

  if (!dhcp && !(testIp.fromString(tmpIp) && testGw.fromString(tmpGw) && testSn.fromString(tmpSn))) {
    debugLogPrint("[WiFi]", "IP inválido ignorado");
    return;
  }

  // só copia se veio algo (ou DHCP explícito)
  if (strlen(tmpIp) > 0) {
    strlcpy(ipStr, tmpIp, sizeof(ipStr));
    strlcpy(gwStr, tmpGw, sizeof(gwStr));
    strlcpy(snStr, tmpSn, sizeof(snStr));
  } else {
    // limpa para DHCP
    ipStr[0] = '\0';
    gwStr[0] = '\0';
    snStr[0] = '\0';
  }

  copyParam(wm, &p_mqtt_server, "mqtt_server", mqtt_server, sizeof(mqtt_server));
  copyParam(wm, &p_mqtt_user, "mqtt_user", mqtt_user_buf, sizeof(mqtt_user_buf));

  char tmp[16];
  if (getParamValue(wm, &p_mqtt_port, "mqtt_port", tmp, sizeof(tmp))) {
    mqtt_port = atoi(tmp);
    if (mqtt_port == 0) mqtt_port = 1883;
  }

  char tmpPass[64];
  if (getParamValue(wm, &p_mqtt_password, "mqtt_password", tmpPass, sizeof(tmpPass))) {
    if (strlen(tmpPass)) {
      strlcpy(mqtt_password_buf, tmpPass, sizeof(mqtt_password_buf));
    }
  }

  char tmpEn[8] = "";
  if (getParamValue(wm, &p_mqtt_enabled, "mqtt_enabled", tmpEn, sizeof(tmpEn))) {
    mqtt_enabled = (strcmp(tmpEn, "1") == 0 || strcasecmp(tmpEn, "true") == 0 || strcasecmp(tmpEn, "on") == 0);
  }

  recalcularTopicos();
  saveConfig();
  ESP.restart();
}

// ==========================
// WATCHDOG DE WIFI
// ==========================
void wifi_watchdog() {

  static unsigned long reconnectStart = 0;
  static bool reconnecting = false;

  // Já conectado
  if (WiFi.status() == WL_CONNECTED) {
    if (reconnecting) {
      reconnecting = false;
      setLedMode(LED_IDLE);
      debugLogPrint("[WiFi]", "Reconectado!");
      debugLogPrintf("[WiFi]", "🌐 IP: %s", WiFi.localIP().toString().c_str());
    }
    return;
  }

  // Não conectado
  if (!reconnecting) {

    setLedMode(LED_WIFI_CONNECTING);

    debugLogPrint("[WiFi]", "Conexão perdida! Tentando reconectar...");

    String ssid = WiFi.SSID();
    String pass = WiFi.psk();

    WiFi.disconnect();
    delay(200);

    // Reaplica IP fixo se configurado
    IPAddress ip, gw, sn, dns(8, 8, 8, 8);
    if (strlen(ipStr) > 6 && ip.fromString(ipStr) && gw.fromString(gwStr) && sn.fromString(snStr)) {

      WiFi.config(ip, gw, sn, dns);
      debugLogPrint("[WiFi]", "IP fixo reaplicado");
    }

    if (ssid.length() > 0) {
      WiFi.begin(ssid, pass);
    } else {
      WiFi.begin();
    }

    reconnectStart = millis();
    reconnecting = true;
    return;
  }

  // Timeout de reconexão
  if (millis() - reconnectStart >= 15000) {

    reconnecting = false;
    setLedMode(LED_WIFI_DISCONNECTED);
    debugLogPrint("[WiFi]", "Falha ao reconectar");
  }
}

// ==========================
// CALLBACK SAVE CONFIG
// ==========================
void saveConfigCallback() {
  debugLogPrint("[WiFiManager]", "Solicitação para salvar config");

  shouldSaveConfig = true;
}

// ==========================
// RECALCULA TÓPICOS MQTT
// ==========================
void recalcularTopicos() {
  myTopic = String(mqtt_id_buf) + "-" + String(grupo_buf);
  clientID = String(myTopic) + "-" + String(ESP.getChipId());
}

void printPortalCredentials() {
  debugLogPrintf("[Portal]", "AUTH SSID: %s Senha  : %s", hostname_buf, PasswordPortal);
}