/* =========================================================
   app.js — IRHub-8266 Frontend
   Compartilhado por todas as páginas (index, system, settings).
   Todas as funções usam getElementById com checagem defensiva
   para funcionar corretamente em páginas que não têm todos os elementos.
========================================================= */

/* =========================================================
   1. ESTADO GLOBAL
   Persistido em window.appState para sobreviver à navegação
   entre páginas sem reconectar o WebSocket.
========================================================= */
const state =
  window.appState ||
  (window.appState = {
    ws: null, // instância do WebSocket
    reconnectTimer: null, // timer de reconexão automática
    wsQueue: [], // fila de mensagens pendentes enquanto WS está offline
    uptimeSeconds: 0, // segundos de uptime sincronizados com o backend
    irHistory: [], // histórico de sinais IR recebidos (máx 10)
    configPopulated: false, // evita repopular o form de config a cada mensagem
    irModeIndex: 0, // índice do modo de recepção IR atual
    irDotTimer: null, // timer do dot de atividade IR
    uptimeInterval: null, // setInterval do contador de uptime
    selectedRemote: null, // modelo de controle remoto selecionado
    saveConfigTimer: null, // timeout de confirmação de saveConfig
  });

/* =========================================================
   2. WEBSOCKET
   Conecta na porta 81. Reconecta automaticamente a cada 2s
   após desconexão. Mensagens enviadas offline ficam na fila
   e são enviadas ao reconectar.
========================================================= */

// Abre a conexão WebSocket se ainda não estiver aberta.
function connectWS() {
  if (state.ws && state.ws.readyState === WebSocket.OPEN) return;

  state.ws = new WebSocket(`ws://${location.hostname}:81`);

  state.ws.onopen = () => {
    updateWSStatus(true);
    flushQueue();
    state.configPopulated = false;
    if (state.reconnectTimer) {
      clearTimeout(state.reconnectTimer);
      state.reconnectTimer = null;
    }
  };

  state.ws.onclose = () => {
    updateWSStatus(false);
    state.configPopulated = false;
    scheduleReconnect();
  };

  state.ws.onerror = () => state.ws.close();
  state.ws.onmessage = handleWSMessage;
}

// Agenda tentativa de reconexão após 2 segundos.
function scheduleReconnect() {
  if (state.reconnectTimer) return;
  state.reconnectTimer = setTimeout(() => {
    state.reconnectTimer = null;
    connectWS();
  }, 2000);
}

// Envia mensagem pelo WS. Se offline, enfileira para envio posterior.
function wsSend(msg) {
  const data = typeof msg === "object" ? JSON.stringify(msg) : msg;
  if (state.ws && state.ws.readyState === WebSocket.OPEN) {
    state.ws.send(data);
  } else {
    state.wsQueue.push(data);
  }
}

// Envia todas as mensagens enfileiradas após reconexão.
function flushQueue() {
  while (state.wsQueue.length && state.ws.readyState === WebSocket.OPEN) {
    state.ws.send(state.wsQueue.shift());
  }
}

/* =========================================================
   3. HANDLER DE MENSAGENS WS
   Roteador central — cada tipo de mensagem dispara
   um updater específico de UI.
========================================================= */
function handleWSMessage(event) {
  let data;
  try {
    data = JSON.parse(event.data);
  } catch {
    return;
  }

  if (!data.type) return;

  switch (data.type) {
    case "system":
      updateSystemWS(data);
      break;
    case "outputs":
      updateOutputsWS(data);
      break;
    case "sensor":
      updateSensorWS(data);
      break;
    case "ir":
      updateIRWS(data);
      break;
    case "ir_receptor":
      updateIRReceptorWS(data);
      break;
    case "network":
      updateNetworkWS(data);
      break;
    case "mqtt":
      updateMQTTWS(data);
      break;

    case "reboot":
      alert("Dispositivo reiniciando...");
      break;

    case "wifiPortal":
      alert("Conecte-se à rede '" + (getText("name") || "irhub8266") + "'");
      break;

    case "wifiReset":
      alert("WiFi resetado!");
      break;

    case "configSaved":
      // Cancela o timeout de confirmação e exibe sucesso
      if (state.saveConfigTimer) {
        clearTimeout(state.saveConfigTimer);
        state.saveConfigTimer = null;
      }
      showCfgStatus("✅ Configuração salva!", "#22c55e");
      break;

    case "configError":
      showCfgStatus("❌ Erro: " + (data.msg || ""), "#ef4444");
      break;

    case "configReset":
      alert("Configurações resetadas!");
      break;

    case "authError":
      alert("❌ Senha incorreta!");
      break;
  }
}

/* =========================================================
   4. HELPERS DE UI
========================================================= */

// Define o textContent de um elemento pelo id, com segurança.
function setText(id, val) {
  const el = document.getElementById(id);
  if (el && val !== undefined && val !== null) el.textContent = val;
}

// Lê o textContent de um elemento pelo id.
function getText(id) {
  return document.getElementById(id)?.textContent || "";
}

/* =========================================================
   5. STATUS DO WEBSOCKET
   Atualiza o badge na navbar e o overlay de reconexão.
========================================================= */
function updateWSStatus(connected) {
  const el = document.getElementById("wsStatus");
  if (el) {
    el.innerHTML = connected
      ? '<span class="status online">⚡ Online</span>'
      : '<span class="status offline">⚡ Offline</span>';
  }

  if (!connected) {
    document.title = `❌ ${getText("name") || "IRHub-8266"}`;
  }

  // Exibe/oculta overlay de reconexão
  const overlay = document.getElementById("wsOverlay");
  if (overlay) overlay.style.display = connected ? "none" : "flex";
}

/* =========================================================
   6. DRAWER (menu mobile)
========================================================= */
function drawerOpen() {
  document.getElementById("drawer")?.classList.add("open");
  document.getElementById("drawerOverlay")?.classList.add("open");
}

function drawerClose() {
  document.getElementById("drawer")?.classList.remove("open");
  document.getElementById("drawerOverlay")?.classList.remove("open");
}

/* =========================================================
   7. UPDATERS DE UI — chamados pelo handler de mensagens WS
========================================================= */

// Atualiza informações de sistema (nome, build, heap, uptime, config).
function updateSystemWS(data) {
  setText("name", data.name);
  setText("buildDateTime", data.buildDateTime);
  setText("buildVersion", data.buildVersion);
  setText("heap", data.heap);

  document.title = `✅ ${data.name}`;

  if (data.uptime_seconds !== undefined) {
    state.uptimeSeconds = data.uptime_seconds;
  }

  // Popula o form de configurações apenas uma vez por conexão
  if (data.config && !state.configPopulated) {
    populateConfig(data.config);
    state.configPopulated = true;
  }

  // Mostra/oculta card AHT10 conforme configuração
  const cardAHT10 = document.getElementById("cardAHT10");
  if (cardAHT10) cardAHT10.style.display = data.aht10_enabled ? "" : "none";
}

// Atualiza estado do LED (dot + texto).
function updateOutputsWS(data) {
  setText("ledState", data.led ? "Ligado" : "Desligado");
  const dot = document.getElementById("ledDot");
  if (dot) dot.className = "dot " + (data.led ? "green" : "red");
}

// Mapa de nome de protocolo IR para índice numérico.
const irModeMap = {
  ALL: 0,
  KNOWN: 1,
  DISABLED: 2,
  NEC: 3,
  SONY: 4,
  RC5: 5,
  RC6: 6,
  SAMSUNG: 7,
  NIKAI: 8,
  LG: 9,
  JVC: 10,
  WHYNTER: 11,
};

// Atualiza estado do emissor e receptor IR.
function updateIRWS(data) {
  setText("irEmitter", data.emissor_teste ? "Ativo" : "Desligado");
  setText("irMode", data.receptor_protocol || "--");

  // Sincroniza select e índice com o protocolo recebido
  if (
    data.receptor_protocol &&
    irModeMap[data.receptor_protocol] !== undefined
  ) {
    state.irModeIndex = irModeMap[data.receptor_protocol];
    const sel = document.getElementById("irReceptorSelect");
    if (sel) sel.value = irModeMap[data.receptor_protocol];
  }

  // Atualiza dot apenas se não há timer ativo (evita piscar durante flash)
  if (!state.irDotTimer) {
    const dot = document.getElementById("irDot");
    if (dot)
      dot.className =
        "dot " +
        (data.receptor_protocol && data.receptor_protocol !== "DESABILITADO"
          ? "green"
          : "yellow");
  }
}

// Atualiza dados do sensor AHT10.
function updateSensorWS(data) {
  const status = document.getElementById("sensorAHT10Status");
  const text = document.getElementById("sensorAHT10Data");
  if (!status || !text) return;

  if (data.status) {
    // Sensor offline ou em erro
    status.className = "status offline";
    status.textContent = data.status;
    text.textContent = "";
    return;
  }

  text.innerHTML = `Temperatura: ${data.temperatura} °C<br>Umidade: ${data.umidade} %`;
  status.className = "status online";
  status.textContent = "online";
}

// Trata sinal IR recebido: pisca dot e adiciona ao histórico.
function updateIRReceptorWS(data) {
  flashIRDot();
  const entry = { ...data, timestamp: new Date().toLocaleTimeString("pt-BR") };
  saveIRToHistory(entry);
}

// Pisca o dot IR por 300ms ao receber um sinal.
function flashIRDot() {
  const dot = document.getElementById("irDot");
  if (!dot) return;

  if (state.irDotTimer) {
    clearTimeout(state.irDotTimer);
    state.irDotTimer = null;
  }

  dot.className = "dot green";
  state.irDotTimer = setTimeout(() => {
    const d = document.getElementById("irDot"); // re-busca após navegação
    if (d) d.className = "dot yellow";
    state.irDotTimer = null;
  }, 300);
}

// Atualiza dados de rede.
function updateNetworkWS(data) {
  ["mdns", "wifi", "ip", "gateway", "mask", "rssi"].forEach((id) =>
    setText(id, data[id]),
  );
}

// Atualiza dados MQTT (status, server, tópicos, contadores).
function updateMQTTWS(data) {
  const cardMQTT = document.getElementById("cardMQTT");
  if (cardMQTT) cardMQTT.style.display = data.enabled ? "" : "none";

  setText("mqttServer", data.server);
  setText("mqttPort", data.port);
  setText("mqttClient", data.client_id);
  setText("topic_main", data.topic_main);
  setText("mqttSucessos", data.sucesso);
  setText("mqttErros", data.erro);

  const status = document.getElementById("mqttStatus");
  if (!status) return;

  if (!data.enabled) {
    status.className = "status warn";
    status.textContent = "desabilitado";
  } else if (data.status) {
    status.className = "status online";
    status.textContent = "online";
  } else {
    status.className = "status offline";
    status.textContent = "offline";
  }
}

/* =========================================================
   8. UPTIME
   Incrementa localmente a cada segundo e sincroniza
   com o valor do backend a cada mensagem "system".
========================================================= */
function formatUptime(s) {
  const d = Math.floor(s / 86400);
  const h = Math.floor((s % 86400) / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  return `${d}d ${String(h).padStart(2, "0")}h ${String(m).padStart(2, "0")}m ${String(sec).padStart(2, "0")}s`;
}

// Inicia o intervalo de uptime apenas uma vez (sobrevive à navegação).
if (!state.uptimeInterval) {
  state.uptimeInterval = setInterval(() => {
    state.uptimeSeconds++;
    setText("uptime", formatUptime(state.uptimeSeconds));
  }, 1000);
}

/* =========================================================
   9. HISTÓRICO IR
   Armazena os últimos 10 sinais recebidos.
   Click simples copia o hex para a área de transferência.
   Double-click carrega no form de envio manual.
========================================================= */

// Adiciona entrada ao histórico, ignorando duplicatas consecutivas.
function saveIRToHistory(payload) {
  if (!payload.dec) return;
  if (state.irHistory[0]?.dec === payload.dec) return;
  state.irHistory.unshift(payload);
  if (state.irHistory.length > 10) state.irHistory.pop();
  renderIRHistory();
}

// Limpa todo o histórico.
function cleanHistory() {
  state.irHistory = [];
  renderIRHistory();
}

// Renderiza a lista de histórico IR no DOM.
function renderIRHistory() {
  const list = document.getElementById("irHistory");
  if (!list) return;

  list.innerHTML = "";

  state.irHistory.forEach((d) => {
    const li = document.createElement("li");
    li.textContent = `${d.timestamp} | ${d.protocol} | ${d.bits}b | ${d.dec} | ${d.hex} 📋`;

    // Click simples: copia hex para clipboard
    li.onclick = () => {
      if (navigator.clipboard) {
        navigator.clipboard.writeText(d.hex).catch(() => {});
      } else {
        // Fallback para browsers sem API de clipboard
        const ta = document.createElement("textarea");
        ta.value = d.hex;
        ta.style.position = "fixed";
        ta.style.opacity = "0";
        document.body.appendChild(ta);
        ta.focus();
        ta.select();
        document.execCommand("copy");
        document.body.removeChild(ta);
      }
    };

    // Double-click: carrega no form de envio manual
    li.ondblclick = () => {
      const proto = document.getElementById("irProto");
      const code = document.getElementById("irCode");
      const bits = document.getElementById("irBits");

      if (proto) proto.value = d.protocol;
      if (code) {
        code.value = d.hex;
        code.placeholder = "Código hex (ex: 0x20DF10EF)";
      }
      if (bits) bits.value = d.bits;

      li.classList.add("flash");
      setTimeout(() => li.classList.remove("flash"), 400);
    };

    list.appendChild(li);
  });
}

/* =========================================================
   10. CONTROLES DE REDE (settings)
========================================================= */

// Exibe/oculta campos de IP fixo conforme o modo selecionado.
function toggleIPFields() {
  const mode = document.getElementById("cfg_ip_mode")?.value;
  const fields = document.getElementById("cfg_ip_fields");
  if (fields) fields.style.display = mode === "static" ? "block" : "none";
}

// Exibe/oculta campos MQTT conforme habilitado/desabilitado.
function toggleMQTTFields() {
  const enabled = document.getElementById("cfg_mqtt_enabled")?.value;
  const fields = document.getElementById("cfg_mqtt_fields");
  if (fields) fields.style.display = enabled === "yes" ? "block" : "none";
}

// Valida formato de endereço IP (aceita vazio = DHCP).
function validarIP(ip) {
  if (ip === "") return true;
  const partes = ip.split(".");
  if (partes.length !== 4) return false;
  return partes.every((p) => {
    const n = parseInt(p);
    return !isNaN(n) && n >= 0 && n <= 255 && String(n) === p;
  });
}

/* =========================================================
   11. CONTROLES REMOTOS (remotes.json)
   Carrega lista de modelos e renderiza botões dinamicamente.
========================================================= */

let remotes = {};

// Carrega remotes.json e popula o select de modelos.
async function loadRemotes() {
  const select = document.getElementById("remoteSelect");
  if (!select) return;

  try {
    const res = await fetch("/remotes.json");
    remotes = await res.json();

    select.innerHTML = "";
    Object.keys(remotes).forEach((model) => {
      const opt = document.createElement("option");
      opt.value = model;
      opt.textContent = model;
      select.appendChild(opt);
    });

    // Restaura modelo salvo no localStorage ou state
    let saved = state.selectedRemote;
    try {
      saved = localStorage.getItem("selectedRemote") || saved;
    } catch (_) {}

    if (saved && remotes[saved]) {
      select.value = saved;
      state.selectedRemote = saved;
    }

    loadButtons(select.value);
  } catch (e) {
    console.error("Erro ao carregar remotes:", e);
  }
}

// Renderiza os botões do modelo selecionado.
function loadButtons(model) {
  const container = document.getElementById("buttonsContainer");
  if (!container || !remotes[model]) return;

  container.innerHTML = "";

  remotes[model].buttons.forEach((btn) => {
    const button = document.createElement("button");

    if (btn.type === "space") {
      // Espaço vazio para alinhar botões no grid
      button.className = "btn-remote";
      button.style.visibility = "hidden";
      button.disabled = true;
    } else {
      button.className = "btn-remote";
      button.textContent = btn.name;
      if (btn.background) button.style.background = `#${btn.background}`;
      if (btn.color) button.style.color = `#${btn.color}`;
      button.onclick = () => {
        sendIR(btn);
        button.classList.add("active");
        setTimeout(() => button.classList.remove("active"), 150);
      };
    }

    container.appendChild(button);
  });

  console.debug(`Modelo Carregado: ${model}`);
}

/* =========================================================
   12. IR — ENVIO
========================================================= */

// Envia código IR de um botão do controle remoto.
function sendIR(btn) {
  if (!btn.code) {
    console.warn("Botão sem campo code:", btn);
    return;
  }
  wsSend({
    cmd: "sendIR",
    code: btn.code,
    protocol: btn.protocol,
    bits: btn.bits || 32,
  });
  console.log("IR enviado:", btn.name);
}

// Envia código IR digitado manualmente no form.
function sendIRManual() {
  const code = document.getElementById("irCode")?.value.trim();
  const proto = document.getElementById("irProto")?.value;
  const bits = parseInt(document.getElementById("irBits")?.value) || 32;

  if (!code) return alert("Digite um código IR.");

  wsSend({ cmd: "sendIR", code, protocol: proto, bits });
}

/* =========================================================
   13. IR — CONTROLES DO RECEPTOR E EMISSOR
========================================================= */

// Alterna o modo de teste do emissor IR.
function toggleIREmissor() {
  wsSend({ cmd: "toggleIREmissor" });
}

// Define o protocolo do receptor IR via select.
function setIRReceptor() {
  const select = document.getElementById("irReceptorSelect");
  if (!select) return;
  wsSend({ cmd: "setIRReceptor", mode: parseInt(select.value) });
}

/* =========================================================
   14. LED
========================================================= */
function toggleLED() {
  wsSend({ cmd: "toggleLED" });
}

/* =========================================================
   15. COMANDOS DO DISPOSITIVO
   Todos os comandos destrutivos pedem confirmação e senha.
========================================================= */

function rebootDevice() {
  if (!confirm("Reiniciar o dispositivo?")) return;
  wsSend({ cmd: "reboot" });
}

function openWifiPortal() {
  if (!confirm("Abrir portal de configuração WiFi?")) return;
  const pass = prompt("Digite a senha para confirmar:");
  if (!pass) return;
  wsSend({ cmd: "wifiPortal", password: pass });
}

function resetWifi() {
  if (!confirm("Resetar configurações WiFi?")) return;
  const pass = prompt("Digite a senha para confirmar:");
  if (!pass) return;
  wsSend({ cmd: "resetWifi", password: pass });
}

function resetConfig() {
  if (!confirm("Reset total? Isso apagará todas as configurações!")) return;
  const pass = prompt("Digite a senha para confirmar:");
  if (!pass) return;
  wsSend({ cmd: "resetConfig", password: pass });
}

/* =========================================================
   16. CONFIGURAÇÃO DO DISPOSITIVO
========================================================= */

// Popula o form de configurações com os dados recebidos do backend.
// Chamado uma vez por conexão WS via updateSystemWS.
function populateConfig(data) {
  const fields = {
    cfg_hostname: data.hostname,
    cfg_mqtt_id: data.mqtt_id,
    cfg_grupo: data.grupo,
    cfg_ip: data.ip,
    cfg_gw: data.gw,
    cfg_sn: data.sn,
    cfg_mqtt_server: data.mqtt_server,
    cfg_mqtt_port: data.mqtt_port,
    cfg_mqtt_user: data.mqtt_user,
    cfg_mqtt_enabled: data.mqtt_enabled,
    cfg_aht10_enabled: data.aht10_enabled ? "true" : "false",
    cfg_wifi_ssid: data.wifi_ssid,
    // cfg_mqtt_password omitido intencionalmente — backend não envia a senha
  };

  Object.entries(fields).forEach(([id, val]) => {
    const el = document.getElementById(id);
    if (el && val !== undefined) el.value = val;
  });

  // Detecta modo IP (DHCP ou fixo) e exibe campos correspondentes
  const modeEl = document.getElementById("cfg_ip_mode");
  if (modeEl) {
    modeEl.value = data.ip ? "static" : "dhcp";
    toggleIPFields();
  }

  // Detecta estado MQTT e exibe campos correspondentes
  const mqttEl = document.getElementById("cfg_mqtt_enabled");
  if (mqttEl) {
    toggleMQTTFields();
  }
}

// Valida e envia o payload de configuração para o backend.
function saveDeviceConfig() {
  const get = (id) => document.getElementById(id)?.value ?? "";
  const password = get("cfg_mqtt_password");

  // Lê campos de IP respeitando o modo selecionado
  const ipMode = document.getElementById("cfg_ip_mode")?.value;
  const ip = ipMode === "static" ? get("cfg_ip").trim() : "";
  const gw = ipMode === "static" ? get("cfg_gw").trim() : "";
  const sn = ipMode === "static" ? get("cfg_sn").trim() : "";

  // Valida IP fixo: todos os campos devem ser preenchidos e válidos
  if (ipMode === "static") {
    if (!ip || !gw || !sn) {
      showCfgStatus("❌ Preencha IP, Gateway e Subnet.", "#ef4444");
      return;
    }
    if (!validarIP(ip) || !validarIP(gw) || !validarIP(sn)) {
      showCfgStatus("❌ IP, Gateway ou Subnet inválidos.", "#ef4444");
      return;
    }
  }

  const payload = {
    cmd: "saveConfig",
    hostname: get("cfg_hostname").trim(),
    mqtt_id: get("cfg_mqtt_id").trim(),
    grupo: get("cfg_grupo").trim(),
    ip,
    gw,
    sn,
    mqtt_server: get("cfg_mqtt_server").trim(),
    mqtt_user: get("cfg_mqtt_user").trim(),
    mqtt_password: password.length > 0 ? password : "__keep__",
    mqtt_enabled: get("cfg_mqtt_enabled"),
    mqtt_port: parseInt(get("cfg_mqtt_port")) || 1883,
    aht10_enabled: get("cfg_aht10_enabled") === "true",
  };

  wsSend(payload);
  showCfgStatus("⏳ Salvando...", "#facc15");

  // Timeout de confirmação — avisa se não receber configSaved em 5s
  if (state.saveConfigTimer) clearTimeout(state.saveConfigTimer);
  state.saveConfigTimer = setTimeout(() => {
    state.saveConfigTimer = null;
    showCfgStatus(
      "⚠️ Sem confirmação do dispositivo. Verifique a conexão.",
      "#f59e0b",
    );
  }, 5000);
}

// Exibe mensagem de status no form de configurações por 3 segundos.
function showCfgStatus(msg, color) {
  const el = document.getElementById("cfgStatus");
  if (!el) return;
  el.textContent = msg;
  el.style.color = color;
  setTimeout(() => (el.textContent = ""), 3000);
}

/* =========================================================
   17. INICIALIZAÇÃO
   Executado uma vez ao carregar cada página.
========================================================= */
document.addEventListener("DOMContentLoaded", () => {
  // Cancela timer de dot que pode ter sobrado da página anterior
  if (state.irDotTimer) {
    clearTimeout(state.irDotTimer);
    state.irDotTimer = null;
  }

  setText("uptime", "0d 00h 00m 00s");
  renderIRHistory();

  // Listener do select de modelo de controle remoto — só existe em index.html
  const select = document.getElementById("remoteSelect");
  if (select) {
    select.addEventListener("change", (e) => {
      state.selectedRemote = e.target.value;
      try {
        localStorage.setItem("selectedRemote", e.target.value);
      } catch (_) {}
      loadButtons(e.target.value);
    });
  }

  loadRemotes();
  connectWS();

  // Se WS já estava aberto ao navegar, atualiza badge e título imediatamente
  if (state.ws && state.ws.readyState === WebSocket.OPEN) {
    updateWSStatus(true);
  }
});
