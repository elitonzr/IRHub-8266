/* =========================================================
   GLOBAL STATE (SAFE FOR ESP8266 MULTI-PAGE)
========================================================= */
const state =
  window.appState ||
  (window.appState = {
    ws: null,
    reconnectTimer: null,
    wsQueue: [],
    uptimeSeconds: 0,
    irHistory: [],
    configPopulated: false,
    irModeIndex: 0,
    irDotTimer: null,
    uptimeInterval: null,
    selectedRemote: null,
  });

/* =========================================================
   WEBSOCKET
========================================================= */
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

function scheduleReconnect() {
  if (state.reconnectTimer) return;
  state.reconnectTimer = setTimeout(() => {
    state.reconnectTimer = null;
    connectWS();
  }, 2000);
}

function wsSend(msg) {
  const data = typeof msg === "object" ? JSON.stringify(msg) : msg;
  if (state.ws && state.ws.readyState === WebSocket.OPEN) {
    state.ws.send(data);
  } else {
    state.wsQueue.push(data);
  }
}

function flushQueue() {
  while (state.wsQueue.length && state.ws.readyState === WebSocket.OPEN) {
    state.ws.send(state.wsQueue.shift());
  }
}

/* =========================================================
   MESSAGE HANDLER
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
   UI HELPERS
========================================================= */
function setText(id, val) {
  const el = document.getElementById(id);
  if (el && val !== undefined && val !== null) el.textContent = val;
}

function getText(id) {
  return document.getElementById(id)?.textContent || "";
}

/* =========================================================
   STATUS
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
}

/* =========================================================
   DRAWER
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
   WS UPDATERS
========================================================= */
function updateSystemWS(data) {
  setText("name", data.name);
  setText("buildDateTime", data.buildDateTime);
  setText("buildVersion", data.buildVersion);
  setText("heap", data.heap);

  document.title = `✅ ${data.name}`;

  if (data.uptime_seconds !== undefined) {
    state.uptimeSeconds = data.uptime_seconds;
  }

  if (data.config && !state.configPopulated) {
    populateConfig(data.config);
    state.configPopulated = true;
  }
}

function updateOutputsWS(data) {
  setText("ledState", data.led ? "Ligado" : "Desligado");
  const dot = document.getElementById("ledDot");
  if (dot) dot.className = "dot " + (data.led ? "green" : "red");
}

const irModeMap = {
  DESABILITADO: 0,
  NEC: 1,
  NIKAI: 2,
  "NEC e NIKAI": 3,
  TUDO: 4,
};

function updateIRWS(data) {
  setText("irEmitter", data.emissor_teste ? "Ativo" : "Desligado");
  setText("irMode", data.receptor_protocol || "--");

  if (
    data.receptor_protocol &&
    irModeMap[data.receptor_protocol] !== undefined
  ) {
    state.irModeIndex = irModeMap[data.receptor_protocol];
  }

  if (!state.irDotTimer) {
    const dot = document.getElementById("irDot");
    if (dot)
      dot.className = "dot " + (data.receptor_protocol ? "green" : "yellow");
  }
}

function updateSensorWS(data) {
  const status = document.getElementById("sensorAHT10Status");
  const text = document.getElementById("sensorAHT10Data");
  if (!status || !text) return;

  if (data.status) {
    status.className = "status offline";
    status.textContent = data.status;
    text.textContent = "";
    return;
  }

  text.innerHTML = `Temperatura: ${data.temperatura} °C<br>Umidade: ${data.umidade} %`;
  status.className = "status online";
  status.textContent = "online";
}

function updateIRReceptorWS(data) {
  flashIRDot();
  const entry = { ...data, timestamp: new Date().toLocaleTimeString() };
  saveIRToHistory(entry);
}

function flashIRDot() {
  const dot = document.getElementById("irDot");
  if (!dot) return;
  if (state.irDotTimer) {
    clearTimeout(state.irDotTimer);
    state.irDotTimer = null;
  }
  dot.className = "dot green";
  state.irDotTimer = setTimeout(() => {
    dot.className = "dot yellow";
    state.irDotTimer = null;
  }, 300);
}

function updateNetworkWS(data) {
  ["mdns", "wifi", "ip", "gateway", "mask", "rssi"].forEach((id) =>
    setText(id, data[id]),
  );
}

function updateMQTTWS(data) {
  setText("mqttServer", data.server);
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
   UPTIME
========================================================= */
function formatUptime(s) {
  const d = Math.floor(s / 86400);
  const h = Math.floor((s % 86400) / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  return `${d}d ${String(h).padStart(2, "0")}h ${String(m).padStart(2, "0")}m ${String(sec).padStart(2, "0")}s`;
}

if (!state.uptimeInterval) {
  state.uptimeInterval = setInterval(() => {
    state.uptimeSeconds++;
    setText("uptime", formatUptime(state.uptimeSeconds));
  }, 1000);
}

/* =========================================================
   IR HISTORY
========================================================= */

function saveIRToHistory(payload) {
  if (payload.dec === 0) return;
  if (state.irHistory[0]?.dec === payload.dec) return;
  state.irHistory.unshift(payload);
  if (state.irHistory.length > 10) state.irHistory.pop();
  renderIRHistory();
}

function cleanHistory() {
  state.irHistory = [];
  renderIRHistory();
}

function renderIRHistory() {
  const list = document.getElementById("irHistory");
  if (!list) return;

  list.innerHTML = "";

  state.irHistory.forEach((d) => {
    const li = document.createElement("li");
    li.textContent = `${d.timestamp} | ${d.protocol} | ${d.bits}b | ${d.dec} | ${d.hex} 📋`;

    li.onclick = () => {
      if (navigator.clipboard) {
        navigator.clipboard.writeText(d.hex).catch(() => {});
      } else {
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

    li.ondblclick = () => {
      const proto = document.getElementById("irProto");
      const format = document.getElementById("irFormat");
      const code = document.getElementById("irCode");
      const bits = document.getElementById("irBits");

      if (proto) proto.value = d.protocol;
      if (format) format.value = "hex";
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
   REMOTES (JSON DINÂMICO)
========================================================= */
let remotes = {};

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
    // Restaura modelo salvo, ou usa o primeiro disponível
    const saved =
      localStorage.getItem("selectedRemote") || state.selectedRemote;
    if (saved && remotes[saved]) {
      select.value = saved;
      state.selectedRemote = saved;
    }
    loadButtons(select.value);
  } catch (e) {
    console.error("Erro ao carregar remotes:", e);
  }
}

function loadButtons(model) {
  const container = document.getElementById("buttonsContainer");
  if (!container || !remotes[model]) return;

  container.innerHTML = "";

  remotes[model].buttons.forEach((btn) => {
    const button = document.createElement("button");
    if (btn.type === "space") {
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
   IR — ENVIO
========================================================= */
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

function sendIRManual() {
  const code = document.getElementById("irCode")?.value.trim();
  const proto = document.getElementById("irProto")?.value;
  const bits = parseInt(document.getElementById("irBits")?.value) || 32;

  if (!code) return alert("Digite um código IR.");

  wsSend({
    cmd: "sendIR",
    code: code,
    protocol: proto,
    bits: bits,
  });
}
/* =========================================================
   IR — CONTROLES
========================================================= */
function toggleIREmissor() {
  wsSend({ cmd: "toggleIREmissor" });
}

function toggleIRReceptor() {
  state.irModeIndex = (state.irModeIndex + 1) % 5;
  wsSend({ cmd: "setIRReceptor", mode: state.irModeIndex });
}

/* =========================================================
   LED
========================================================= */
function toggleLED() {
  wsSend({ cmd: "toggleLED" });
}

/* =========================================================
   DEVICE COMMANDS
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
   CONFIG
========================================================= */
function populateConfig(data) {
  const fields = {
    cfg_hostname: data.hostname,
    cfg_mqtt_id: data.mqtt_id,
    cfg_grupo: data.grupo,
    cfg_ip: data.ip,
    cfg_gw: data.gw,
    cfg_sn: data.sn,
    cfg_mqtt_server: data.mqtt_server,
    cfg_mqtt_user: data.mqtt_user,
    cfg_mqtt_enabled: data.mqtt_enabled,
    cfg_wifi_ssid: data.wifi_ssid,
    // cfg_mqtt_password intencionalmente omitido — servidor não envia a senha
  };
  Object.entries(fields).forEach(([id, val]) => {
    const el = document.getElementById(id);
    if (el && val !== undefined) el.value = val;
  });
}

function saveDeviceConfig() {
  const get = (id) => document.getElementById(id)?.value ?? "";
  const password = get("cfg_mqtt_password");

  const payload = {
    cmd: "saveConfig",
    hostname: get("cfg_hostname").trim(),
    mqtt_id: get("cfg_mqtt_id").trim(),
    grupo: get("cfg_grupo").trim(),
    ip: get("cfg_ip").trim(),
    gw: get("cfg_gw").trim(),
    sn: get("cfg_sn").trim(),
    mqtt_server: get("cfg_mqtt_server").trim(),
    mqtt_user: get("cfg_mqtt_user").trim(),
    mqtt_password: password.length > 0 ? password : "__keep__",
    mqtt_enabled: get("cfg_mqtt_enabled"),
  };

  wsSend(payload);
  showCfgStatus("⏳ Salvando...", "#facc15");
}

function showCfgStatus(msg, color) {
  const el = document.getElementById("cfgStatus");
  if (!el) return;
  el.textContent = msg;
  el.style.color = color;
  setTimeout(() => (el.textContent = ""), 3000);
}

/* =========================================================
   INIT
========================================================= */

document.addEventListener("DOMContentLoaded", () => {
  setText("uptime", "0d 00h 00m 00s");
  renderIRHistory();

  const irFormat = document.getElementById("irFormat");
  if (irFormat) {
    irFormat.addEventListener("change", function () {
      const input = document.getElementById("irCode");
      if (!input) return;
      input.placeholder =
        this.value === "dec"
          ? "Código decimal (ex: 551489775)"
          : "Código hex (ex: 0x20DF10EF)";
      input.value = "";
    });
  }

  const select = document.getElementById("remoteSelect");
  if (select) {
    select.addEventListener("change", (e) => {
      state.selectedRemote = e.target.value;
      localStorage.setItem("selectedRemote", e.target.value);
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
