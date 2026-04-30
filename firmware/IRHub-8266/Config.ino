// ==========================
// CARREGA CONFIG DO FS
// ==========================
void loadConfig() {

  if (!LittleFS.exists("/config.json")) {
    debugLogPrint("[FS]", "config.json não existe");
    recalcularTopicos();
    return;
  }
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    recalcularTopicos();
    return;
  }

  StaticJsonDocument<768> doc;
  DeserializationError err = deserializeJson(doc, file);
  if (err) {
    debugLogPrintf("[FS]", "Erro ao parsear config.json: %s", err.c_str());
  } else {
    strlcpy(hostname_buf, doc["hostname"] | hostname_buf, sizeof(hostname_buf));
    strlcpy(mqtt_id_buf, doc["mqtt_id"] | mqtt_id_buf, sizeof(mqtt_id_buf));
    strlcpy(grupo_buf, doc["grupo"] | grupo_buf, sizeof(grupo_buf));

    // ================================================================
    // REDE
    // ================================================================
    strlcpy(wifi_ssid_buf, doc["wifi_ssid"] | wifi_ssid_buf, sizeof(wifi_ssid_buf));
    strlcpy(wifi_password_buf, doc["wifi_password"] | wifi_password_buf, sizeof(wifi_password_buf));
    strlcpy(ipStr, doc["ip"] | ipStr, sizeof(ipStr));
    strlcpy(gwStr, doc["gw"] | gwStr, sizeof(gwStr));
    strlcpy(snStr, doc["sn"] | snStr, sizeof(snStr));

    strlcpy(mqtt_server, doc["mqtt_server"] | mqtt_server, sizeof(mqtt_server));
    mqtt_port = doc["mqtt_port"] | 1883;
    strlcpy(mqtt_user_buf, doc["mqtt_user"] | mqtt_user_buf, sizeof(mqtt_user_buf));
    strlcpy(mqtt_password_buf, doc["mqtt_password"] | mqtt_password_buf, sizeof(mqtt_password_buf));

    mqtt_enabled = doc["mqtt_enabled"] | false;
    aht10_enabled = doc["aht10_enabled"] | false;

    strlcpy(PasswordWS, doc["ws_password"] | PasswordWS, sizeof(PasswordWS));

    int ir_receptor = doc["ir_receptor"] | (int)IR_PROTOCOL_KNOWN;
    if (ir_receptor >= 0 && ir_receptor <= 11) {
      IR_ReceptorEstado = static_cast<IR_ReceptorMode>(ir_receptor);
    }

    debugLogPrint("[FS]", "Config carregada");
  }
  file.close();
  recalcularTopicos();
}

// ==========================
// SALVA CONFIG NO FS
// ==========================
void saveConfig() {
  StaticJsonDocument<768> doc;

  // -------- Identificação --------
  doc["hostname"] = hostname_buf;
  doc["mqtt_id"] = mqtt_id_buf;
  doc["grupo"] = grupo_buf;

  // -------- REDE --------
  doc["wifi_ssid"] = wifi_ssid_buf;
  doc["wifi_password"] = wifi_password_buf;
  doc["ip"] = ipStr;
  doc["gw"] = gwStr;
  doc["sn"] = snStr;

  // -------- MQTT --------
  doc["mqtt_server"] = mqtt_server;
  doc["mqtt_port"] = mqtt_port;
  doc["mqtt_user"] = mqtt_user_buf;
  doc["mqtt_password"] = mqtt_password_buf;
  doc["mqtt_enabled"] = mqtt_enabled;

  doc["aht10_enabled"] = aht10_enabled;
  doc["ws_password"] = PasswordWS;
  doc["ir_receptor"] = (int)IR_ReceptorEstado;

  // Backup do config anterior
  if (LittleFS.exists("/config.json")) {
    if (LittleFS.exists("/config.bak.json")) LittleFS.remove("/config.bak.json");
    LittleFS.rename("/config.json", "/config.bak.json");
  }

  File file = LittleFS.open("/config.json", "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
  debugLogPrint("[FS]", "Config salva!");
}

// ==========================
// RESET CONFIG NO FS
// ==========================
void resetConfig() {
  if (LittleFS.exists("/config.json")) {
    LittleFS.remove("/config.json");
    debugLogPrint("[FS]", "config.json removido.");
  }
  delay(1000);
  ESP.restart();
}

void handleResetButton() {
  resetConfig();
}

// ==========================
// RESET WIFI NO FS
// ==========================
void resetWifi() {
  wifi_ssid_buf[0] = '\0';
  wifi_password_buf[0] = '\0';
  saveConfig();
  delay(1000);
  ESP.restart();
}