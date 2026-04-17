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
    logHistory: [], // histórico de logs em tempo real (máx 50)
    configPopulated: false, // evita repopular o form de config a cada mensagem
    irModeIndex: 0, // índice do modo de recepção IR atual
    irDotTimer: null, // timer do dot de atividade IR
    uptimeInterval: null, // setInterval do contador de uptime
    saveConfigTimer: null, // timeout de confirmação de saveConfig
    remotesData: {}, // dados do remotes.json carregados em memória
    selectedRemote: localStorage.getItem("selectedRemote") || null, // último modelo selecionado
    lastPayloads: {}, // cache dos últimos payloads recebidos por type
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
   3. ROTEAMENTO SPA
   Carrega partials HTML por rota sem recarregar a página.
========================================================= */

const routes = {
  "/": "/home_content.html",
  "/ir": "/ir_content.html",
  "/system": "/system_content.html",
  "/settings": "/settings_content.html",
};

async function navigateTo(path) {
  const contentArea = document.getElementById("app-content");
  if (!contentArea) return;

  window.history.pushState({}, "", path);

  try {
    const file = routes[path] != null ? routes[path] : routes["/"];
    const response = await fetch(file);

    if (!response.ok) {
      const msg = `fetch(${file}) → HTTP ${response.status} ${response.statusText}`;
      contentArea.innerHTML = `<div style="padding:20px;font-family:monospace"><b>Erro ao carregar página</b><br><span style="font-size:12px;opacity:0.6">${msg}</span></div>`;
      console.error("navigateTo:", msg);
      return;
    }

    contentArea.innerHTML = await response.text();
    initPageScript(path);
    updateActiveLinks(path);
    replayLastPayloads();
  } catch (err) {
    contentArea.innerHTML = `<div style="padding:20px;font-family:monospace"><b>Erro ao carregar página</b><br><span style="font-size:12px;opacity:0.6">${err}</span></div>`;
    console.error("navigateTo exception:", err);
  }
}

// Escuta o botão "Voltar" do navegador.
window.onpopstate = () => navigateTo(window.location.pathname);

// Intercepta cliques nos links da Navbar e Drawer.
document.addEventListener("click", (e) => {
  if (e.target.matches("[data-link]")) {
    e.preventDefault();
    navigateTo(e.target.getAttribute("href"));
  }
});

// Inicializa scripts específicos de cada rota após injeção do HTML.
function initPageScript(path) {
  if (path === "/" || path === "/index.html") {
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
  }

  if (path === "/system") {
    renderLogHistory();
  }

  if (path === "/settings") {
    state.configPopulated = false;
  }
}

// Marca o link ativo na navbar e no drawer conforme o path atual.
function updateActiveLinks(path) {
  document.querySelectorAll("[data-link]").forEach((link) => {
    link.classList.remove("active");
    const linkPath = new URL(link.href, window.location.origin).pathname;
    if (linkPath === path) link.classList.add("active");
  });
}

/* =========================================================
   4. HANDLER DE MENSAGENS WS
   Roteador central — cada type dispara um updater de UI.
========================================================= */

function handleWSMessage(event) {
  let data;
  try {
    data = JSON.parse(event.data);
  } catch {
    return;
  }

  if (data.type) state.lastPayloads[data.type] = data;

  if (!data.type) return;

  switch (data.type) {
    case "system":
      updateSystemWS(data);
      break;
    case "ledb":
      updateLEDBWS(data);
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

    case "log":
      saveLogToHistory(data.msg);
      break;
  }
}

/* =========================================================
   5. HELPERS DE UI
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
   6. STATUS DO WEBSOCKET
   Atualiza o badge na navbar e o overlay de reconexão.
========================================================= */

function updateWSStatus(connected) {
  const el = document.getElementById("wsStatus");
  if (el) {
    el.innerHTML = connected
      ? '<span class="status online">⚡ Online</span>'
      : '<span class="status offline">⚡ Offline</span>';
  }

  if (!connected) document.title = `❌ ${getText("name") || "IRHub-8266"}`;

  const overlay = document.getElementById("wsOverlay");
  if (overlay) overlay.style.display = connected ? "none" : "flex";
}

/* =========================================================
   7. DRAWER (menu mobile)
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
   8. UPDATERS DE UI — chamados pelo handler de mensagens WS
========================================================= */

// Atualiza informações de sistema (nome, build, heap, uptime, config).
function updateSystemWS(data) {
  setText("name", data.name);
  setText("buildDateTime", data.buildDateTime);
  setText("buildVersion", data.buildVersion);
  setText("heap", data.heap);

  document.title = `✅ ${data.name}`;

  if (data.uptime_seconds !== undefined)
    state.uptimeSeconds = data.uptime_seconds;

  // Popula o form de configurações apenas uma vez por conexão.
  if (data.config && !state.configPopulated) {
    populateConfig(data.config);
    state.configPopulated = true;
  }

  // Mostra/oculta card AHT10 conforme configuração.
  const cardAHT10 = document.getElementById("cardAHT10");
  if (cardAHT10)
    cardAHT10.style.display = data.config.aht10_enabled ? "" : "none";
}

// Atualiza estado do LED B (dot + texto).
function updateLEDBWS(data) {
  setText("ledbState", data.state ? "Ligado" : "Desligado");
  const dot = document.getElementById("ledbDot");
  if (dot) dot.className = "dot " + (data.state ? "yellow" : "gray");
}

// Mapa de nome de protocolo IR para índice numérico (espelhado no backend).
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

  if (
    data.receptor_protocol &&
    irModeMap[data.receptor_protocol] !== undefined
  ) {
    state.irModeIndex = irModeMap[data.receptor_protocol];
    const sel = document.getElementById("irReceptorSelect");
    if (sel) sel.value = irModeMap[data.receptor_protocol];
  }

  // Atualiza dot apenas se não há timer ativo (evita conflito com flash).
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
  saveIRToHistory({
    ...data,
    timestamp: new Date().toLocaleTimeString("pt-BR"),
  });
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
    const d = document.getElementById("irDot"); // re-busca após possível navegação
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

// Reaplica o último estado conhecido de cada tipo ao navegar entre páginas.
function replayLastPayloads() {
  const updaters = {
    system: updateSystemWS,
    outputs: updateLEDBWS,
    sensor: updateSensorWS,
    ir: updateIRWS,
    // ir_receptor: updateIRReceptorWS,
    network: updateNetworkWS,
    mqtt: updateMQTTWS,
  };
  Object.entries(updaters).forEach(([type, fn]) => {
    if (state.lastPayloads[type]) fn(state.lastPayloads[type]);
  });
}

/* =========================================================
   9. UPTIME
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

// Inicia o intervalo apenas uma vez — sobrevive à navegação entre páginas.
if (!state.uptimeInterval) {
  state.uptimeInterval = setInterval(() => {
    state.uptimeSeconds++;
    setText("uptime", formatUptime(state.uptimeSeconds));
  }, 1000);
}

/* =========================================================
   10. HISTÓRICO IR
   Armazena os últimos 10 sinais recebidos.
   Click simples: copia o hex para a área de transferência.
   Double-click: carrega no form de envio manual.
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

    // Click simples: copia hex para clipboard.
    li.onclick = () => {
      if (navigator.clipboard) {
        navigator.clipboard.writeText(d.hex).catch(() => {});
      } else {
        // Fallback para browsers sem Clipboard API.
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

    // Double-click: carrega no form de envio manual.
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
   11. HISTÓRICO DE LOGS (Console)
   Armazena as últimas 50 mensagens recebidas via WS "log".
========================================================= */

// Adiciona mensagem ao histórico e re-renderiza.
function saveLogToHistory(msg) {
  if (!msg) return;
  const timestamp = new Date().toLocaleTimeString("pt-BR");
  state.logHistory.push(`${timestamp} ${msg}`);
  if (state.logHistory.length > 50) state.logHistory.shift();
  renderLogHistory();
}

// Renderiza o histórico de logs no DOM.
function renderLogHistory() {
  const list = document.getElementById("logHistory");
  if (!list) return;

  list.innerHTML = "";
  state.logHistory.forEach((msg) => {
    const li = document.createElement("li");
    li.textContent = msg;
    list.appendChild(li);
  });

  list.scrollTop = list.scrollHeight;
}

// Limpa o histórico de logs.
function clearLogs() {
  state.logHistory = [];
  const list = document.getElementById("logHistory");
  if (list) list.innerHTML = "";
}

// Envia comando de texto digitado no console via WS.
function sendTelnetCmd() {
  const input = document.getElementById("telnetInput");
  if (!input) return;
  const line = input.value.trim();
  if (!line) return;
  wsSend({ cmd: "telnetCmd", line });
  input.value = "";
}

/* =========================================================
   12. GERENCIAMENTO DE ARQUIVOS (config.json / remotes.json)
========================================================= */

function exportConfig() {
  window.location.href = "/download?file=/config.json";
}

async function importConfig() {
  if (!confirm("⚠️ O config.json contém dados sensíveis. Deseja continuar?"))
    return;

  const pass = prompt("Digite a senha para confirmar:");
  if (!pass) return;

  const file = document.getElementById("configFile")?.files[0];
  if (!file) return alert("Selecione um arquivo .json");

  // Valida JSON antes de enviar.
  try {
    JSON.parse(await file.text());
  } catch {
    showConfigStatus("❌ Arquivo JSON inválido.", "#ef4444");
    return;
  }

  const formData = new FormData();
  formData.append("upload", file);

  const xhr = new XMLHttpRequest();
  xhr.withCredentials = true;
  xhr.open("POST", "/upload", true);
  xhr.setRequestHeader("Authorization", "Basic " + btoa("admin:" + pass));
  xhr.onload = () => {
    if (xhr.status === 401) {
      showConfigStatus("❌ Senha incorreta.", "#ef4444");
      return;
    }
    showConfigStatus("✅ Importado com sucesso!", "#22c55e");
  };
  xhr.onerror = () => showConfigStatus("❌ Erro no upload.", "#ef4444");
  xhr.send(formData);
}

function showConfigStatus(msg, color) {
  const el = document.getElementById("configFileStatus");
  if (!el) return;
  el.textContent = msg;
  el.style.color = color;
  setTimeout(() => (el.textContent = ""), 3000);
}

function exportRemotes() {
  window.location.href = "/download?file=/remotes.json";
}

async function importRemotes() {
  const file = document.getElementById("remotesFile")?.files[0];
  if (!file) return alert("Selecione um arquivo .json");

  // Valida JSON antes de enviar.
  try {
    JSON.parse(await file.text());
  } catch {
    showRemotesStatus("❌ Arquivo JSON inválido.", "#ef4444");
    return;
  }

  const formData = new FormData();
  formData.append("upload", file);

  const xhr = new XMLHttpRequest();
  xhr.withCredentials = true;
  xhr.open("POST", "/upload", true);
  xhr.onload = () => {
    showRemotesStatus("✅ Importado com sucesso!", "#22c55e");
    loadRemotes(); // Recarrega os botões na UI imediatamente.
  };
  xhr.onerror = () => showRemotesStatus("❌ Erro no upload.", "#ef4444");
  xhr.send(formData);
}

function showRemotesStatus(msg, color) {
  const el = document.getElementById("remotesStatus");
  if (!el) return;
  el.textContent = msg;
  el.style.color = color;
  setTimeout(() => (el.textContent = ""), 3000);
}

/* =========================================================
   13. CONTROLES DE REDE (settings)
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

// Valida formato de endereço IP (string vazia = DHCP, aceito).
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
   14. CONTROLES REMOTOS (remotes.json)
   Carrega lista de modelos e renderiza botões dinamicamente.
========================================================= */

// Busca remotes.json do dispositivo e popula o select de modelos.
async function loadRemotes() {
  try {
    const response = await fetch("/remotes.json");
    if (!response.ok) throw new Error("Erro ao baixar remotes.json");
    state.remotesData = await response.json();
    populateRemoteSelect();
  } catch (err) {
    console.error("Erro ao carregar remotes:", err);
  }
}

// Popula o <select> com os modelos disponíveis e carrega o último selecionado.
function populateRemoteSelect() {
  const select = document.getElementById("remoteSelect");
  if (!select || !state.remotesData) return;

  select.innerHTML = "";

  const models = Object.keys(state.remotesData);
  models.forEach((model) => {
    const opt = document.createElement("option");
    opt.value = model;
    opt.textContent = model;
    select.appendChild(opt);
  });

  const initialModel = state.selectedRemote || models[0];
  if (initialModel && state.remotesData[initialModel]) {
    select.value = initialModel;
    loadButtons(initialModel);
  }
}

// Renderiza os botões do modelo selecionado no grid.
function loadButtons(model) {
  if (!state.remotesData || !state.remotesData[model]) {
    console.warn(`Dados para o modelo ${model} ainda não disponíveis.`);
    return;
  }

  const container = document.getElementById("buttonsContainer");
  if (!container) return;

  container.innerHTML = "";

  const buttons =
    state.remotesData[model].buttons || state.remotesData[model].button || [];

  buttons.forEach((btn) => {
    if (btn.type === "space") {
      const space = document.createElement("div");
      if (btn.span) space.className = `span-${btn.span}`;
      container.appendChild(space);
      return;
    }

    const b = document.createElement("button");
    b.textContent = btn.name;
    b.className = "btn-ir-remote";

    if (btn.fontSize) b.style.fontSize = btn.fontSize;
    if (btn.span) b.classList.add(`span-${btn.span}`);
    if (btn.rowSpan) b.classList.add(`row-span-${btn.rowSpan}`);
    if (btn.background) b.style.background = `#${btn.background}`;
    if (btn.color) b.style.color = `#${btn.color}`;

    b.onclick = () => sendIR(btn.protocol, btn.code, btn.bits, b);
    container.appendChild(b);
  });
}

/* =========================================================
   15. IR — ENVIO
========================================================= */

// Envia código IR de um botão do controle remoto.
function sendIR(protocol, code, bits, element = null) {
  if (element) {
    element.classList.add("btn-active-feedback");
    setTimeout(() => element.classList.remove("btn-active-feedback"), 150);
  }

  if (!code) {
    console.warn("Botão sem campo code:", code);
    showIrToast("Erro: Código ausente!", true);
    return;
  }

  showIrToast(`Enviando ${protocol}...`);
  // wsSend({ cmd: "sendIR", code, protocol, bits: parseInt(bits) || 32 });
  wsSend({
    cmd: "sendIR",
    code: String(code),
    protocol,
    bits: parseInt(bits) || 32,
  });
}

// Exibe toast de feedback de envio IR (roxo = ok, vermelho = erro).
function showIrToast(message, isError = false) {
  const existing = document.querySelector(".ir-sending-toast");
  if (existing) existing.remove();

  const toast = document.createElement("div");
  toast.className = "ir-sending-toast";
  if (isError) toast.classList.add("error");

  toast.textContent = message;
  document.body.appendChild(toast);
  setTimeout(() => toast.remove(), 1000);
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
   16. IR — CONTROLES DO RECEPTOR E EMISSOR
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
   17. LED
========================================================= */

function toggleLEDB() {
  wsSend({ cmd: "toggleLEDB" });
}

/* =========================================================
   18. COMANDOS DO DISPOSITIVO
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
  if (!confirm("Apagar config.json? Isso apagará todas as configurações!"))
    return;
  const pass = prompt("Digite a senha para confirmar:");
  if (!pass) return;
  wsSend({ cmd: "resetConfig", password: pass });
}

/* =========================================================
   19. CONFIGURAÇÃO DO DISPOSITIVO
========================================================= */

// Popula o form de configurações com os dados recebidos do backend.
// Chamado uma vez por conexão WS (via updateSystemWS).
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
    // cfg_mqtt_password omitido — backend não envia a senha
  };

  Object.entries(fields).forEach(([id, val]) => {
    const el = document.getElementById(id);
    if (el && val !== undefined) el.value = val;
  });

  // Detecta modo IP (DHCP ou fixo) e exibe campos correspondentes.
  const modeEl = document.getElementById("cfg_ip_mode");
  if (modeEl) {
    modeEl.value = data.ip ? "static" : "dhcp";
    toggleIPFields();
  }

  // Sincroniza visibilidade dos campos MQTT.
  if (document.getElementById("cfg_mqtt_enabled")) toggleMQTTFields();
}

// Valida e envia o payload de configuração para o backend.
function saveDeviceConfig() {
  const get = (id) => document.getElementById(id)?.value ?? "";
  const password = get("cfg_mqtt_password");

  const ipMode = document.getElementById("cfg_ip_mode")?.value;
  const ip = ipMode === "static" ? get("cfg_ip").trim() : "";
  const gw = ipMode === "static" ? get("cfg_gw").trim() : "";
  const sn = ipMode === "static" ? get("cfg_sn").trim() : "";

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

  wsSend({
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
  });

  showCfgStatus("⏳ Salvando...", "#facc15");

  // Timeout de confirmação — avisa se não receber configSaved em 5s.
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
   20. INICIALIZAÇÃO
   Executado uma vez ao carregar a página.
========================================================= */

document.addEventListener("DOMContentLoaded", () => {
  // Cancela timer de dot que pode ter sobrado de navegação anterior.
  if (state.irDotTimer) {
    clearTimeout(state.irDotTimer);
    state.irDotTimer = null;
  }

  setText("uptime", "0d 00h 00m 00s");
  renderIRHistory();
  renderLogHistory();

  const currentPath = window.location.pathname;
  navigateTo(routes[currentPath] ? currentPath : "/");

  connectWS();

  // Se WS já estava aberto (navegação SPA), atualiza badge imediatamente.
  if (state.ws && state.ws.readyState === WebSocket.OPEN) updateWSStatus(true);
});
