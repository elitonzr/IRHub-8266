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
    ws: null,                                         // instância do WebSocket
    reconnectTimer: null,                             // timer de reconexão automática
    wsQueue: [],                                      // fila de mensagens pendentes enquanto WS está offline
    uptimeSeconds: 0,                                 // segundos de uptime sincronizados com o backend
    irHistory: [],                                    // histórico de sinais IR recebidos (máx 10)
    logHistory: [],                                   // histórico de logs em tempo real (máx 200)
    configPopulated: false,                           // evita repopular o form de config a cada mensagem
    irModeIndex: 0,                                   // índice do modo de recepção IR atual
    irDotTimer: null,                                 // timer do dot de atividade IR
    uptimeInterval: null,                             // setInterval do contador de uptime
    saveConfigTimer: null,                            // timeout de confirmação de saveConfig
    remotesData: {},                                  // dados do remotes.json carregados em memória
    lastPayloads: {},                                 // cache dos últimos payloads recebidos por type
    lastConfig: null,                                 // última config recebida via WS (type: system)
    logRenderedIndex: 0,                              // índice de renderização incremental do console
    _settingsFormDirty: false,                        // true se o usuário editou o form antes de salvar
    selectedRemote: lsGet("selectedRemote") || null,  // último modelo de controle remoto selecionado
    wsAuthenticated: false,                           // true após login WS bem-sucedido
  });

// Lê valor do localStorage com fallback seguro.
function lsGet(key, fallback = "") {
  try { return localStorage.getItem(key) ?? fallback; }
  catch { return fallback; }
}

// Grava valor no localStorage com segurança.
function lsSet(key, val) {
  try { localStorage.setItem(key, val); } catch {}
}

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
    if (state.reconnectTimer) {
      clearTimeout(state.reconnectTimer);
      state.reconnectTimer = null;
    }
    flushQueue();
  };

  state.ws.onclose = () => {
    updateWSStatus(false);
    state.wsAuthenticated = false;
    state.configPopulated = false;
    scheduleReconnect();
  };

  state.ws.onerror   = () => state.ws.close();
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
   2a. AUTENTICAÇÃO
   Login/logout via WS. Modal exibido ao clicar em "Login"
   na navbar. Credenciais: admin_user + PasswordWS do backend.
========================================================= */

function showLoginModal() {
  const modal = document.getElementById("loginModal");
  if (modal) modal.style.display = "flex";
}

function hideLoginModal() {
  const modal = document.getElementById("loginModal");
  if (modal) modal.style.display = "none";
}

// Envia credenciais ao backend via WS para autenticação.
function submitLogin() {
  const user = document.getElementById("loginUser")?.value || "";
  const pass = document.getElementById("loginPass")?.value || "";
  if (!user || !pass) return;
  wsSend({ cmd: "login", user, password: pass });
}

function doLogout() {
  wsSend({ cmd: "logout" });
}

// Alterna entre exibir o modal de login ou enviar logout conforme estado atual.
function handleLoginBtn() {
  if (state.wsAuthenticated) doLogout();
  else showLoginModal();
}

// Atualiza o botão Login/Logoff na navbar e recarrega a rota atual
// para aplicar restrições de visibilidade conforme estado de autenticação.
function updateAuthUI(authenticated) {
  const btn = document.getElementById("btnLogin");
  if (btn) btn.textContent = authenticated ? "Logoff" : "Login";
  navigateTo(window.location.pathname);
}

/* =========================================================
   3. ROTEAMENTO SPA
   Carrega partials HTML por rota sem recarregar a página.
   Rotas protegidas (/ir, /system, /settings) redirecionam
   para / se o usuário não estiver autenticado.
========================================================= */

const routes = {
  "/":         "/home_content.html",
  "/ir":       "/ir_content.html",
  "/system":   "/system_content.html",
  "/settings": "/settings_content.html",
};

async function navigateTo(path) {
  const contentArea = document.getElementById("app-content");
  if (!contentArea) return;

  // Bloqueia acesso a rotas protegidas sem autenticação.
  const protectedRoutes = ["/ir", "/system", "/settings"];
  if (!state.wsAuthenticated && protectedRoutes.includes(path)) path = "/";

  window.history.pushState({}, "", path);

  try {
    const file = routes[path];
    if (!file) {
      contentArea.innerHTML = `<div style="padding:20px">Página não encontrada: ${path}</div>`;
      return;
    }

    const response = await fetch(file);
    if (!response.ok) {
      const msg = `fetch(${file}) → HTTP ${response.status} ${response.statusText}`;
      contentArea.innerHTML = `<div style="padding:20px;font-family:monospace"><b>Erro ao carregar página</b><br><span style="font-size:12px;opacity:0.6">${msg}</span></div>`;
      console.error("navigateTo:", msg);
      return;
    }

    contentArea.innerHTML = await response.text();
    await Promise.resolve(); // garante que o DOM foi atualizado antes de initPageScript
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
    // Vincula evento de troca de modelo ao select de controles remotos.
    const select = document.getElementById("remoteSelect");
    if (select) {
      select.addEventListener("change", (e) => {
        state.selectedRemote = e.target.value;
        lsSet("selectedRemote", e.target.value);
        loadButtons(e.target.value);
      });
    }
    loadRemotes();

    // Oculta o gerenciador de arquivos se não autenticado.
    const fileManager = document.querySelector(".card-file-manager");
    if (fileManager) fileManager.style.display = state.wsAuthenticated ? "" : "none";
  }

  if (path === "/system") {
    // Reinicia o índice de renderização para exibir o histórico completo.
    state.logRenderedIndex = 0;
    renderLogHistory();
  }

  if (path === "/settings") {
    // Repopula o form apenas se não houver edições pendentes do usuário.
    if (!state._settingsFormDirty) {
      state.configPopulated = false;
      if (state.lastConfig) {
        populateConfig(state.lastConfig);
        state.configPopulated = true;
      }
    }
    state._settingsFormDirty = false;

    // Marca o form como sujo ao primeiro input do usuário.
    document.querySelectorAll("#app-content input, #app-content select").forEach((el) => {
      el.addEventListener("change", () => {
        state._settingsFormDirty = true;
        el.classList.add("field-dirty");
      });
    });
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
  try { data = JSON.parse(event.data); }
  catch { return; }

  // Armazena o último payload de cada type para replayLastPayloads().
  if (data.type) state.lastPayloads[data.type] = data;
  if (!data.type) return;

  switch (data.type) {
    case "loginOk":
      state.wsAuthenticated = true;
      hideLoginModal();
      // Só reseta configPopulated se o form não tiver edições pendentes.
      if (!state._settingsFormDirty) state.configPopulated = false;
      replayLastPayloads();
      updateAuthUI(true);
      break;

    case "loginError":
      state.wsAuthenticated = false;
      const errEl = document.getElementById("loginError");
      if (errEl) errEl.style.display = "block";
      break;

    case "logoutOk":
      state.wsAuthenticated = false;
      updateAuthUI(false);
      navigateTo("/");
      break;

    case "system":      updateSystemWS(data);    break;
    case "ledb":        updateLEDBWS(data);       break;
    case "sensor":      updateSensorWS(data);     break;
    case "ir":          updateIRWS(data);         break;
    case "ir_receptor": updateIRReceptorWS(data); break;
    case "network":     updateNetworkWS(data);    break;
    case "mqtt":        updateMQTTWS(data);       break;
    case "console":     saveLogToHistory(data);   break;

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

// Exibe mensagem de status num elemento pelo id, com cor e auto-limpeza.
// timeout=0 mantém a mensagem indefinidamente (usado no OTA).
function showStatus(id, msg, color, timeout = 3000) {
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = msg;
  el.style.color  = color;
  if (timeout > 0) setTimeout(() => (el.textContent = ""), timeout);
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
  setText("name",          data.name);
  setText("buildDateTime", data.buildDateTime);
  setText("buildVersion",  data.buildVersion);
  setText("heap",          data.heap);

  document.title = `✅ ${data.name}`;

  // Exibe badge de "firmware atualizado" se a versão mudou desde a última visita.
  const knownVersion = lsGet("knownVersion");
  if (data.buildVersion && data.buildVersion !== knownVersion) {
    lsSet("knownVersion", data.buildVersion);
    if (knownVersion !== "") {
      const badge = document.getElementById("versionBadge");
      if (badge) {
        badge.textContent  = `✨ v${data.buildVersion}`;
        badge.style.display = "inline";
        badge.title        = `Firmware atualizado! (anterior: v${knownVersion})`;
      }
    }
  }

  // Armazena config para uso em navegações futuras (ex: ao abrir /settings).
  if (data.config) state.lastConfig = data.config;

  // Popula o form de configurações apenas uma vez por conexão.
  if (data.config && !state.configPopulated) {
    populateConfig(data.config);
    state.configPopulated = true;
  }

  if (data.uptime_seconds !== undefined) {
    state.uptimeSeconds = data.uptime_seconds;
    restartUptimeInterval();
  }

  // Mostra/oculta card AHT10 conforme configuração.
  const cardAHT10 = document.getElementById("cardAHT10");
  if (cardAHT10 && data.config)
    cardAHT10.style.display = data.config.aht10_enabled ? "" : "none";
}

// Atualiza estado do LED B (dot + texto).
function updateLEDBWS(data) {
  setText("ledbState", data.state ? "Ligado" : "Desligado");
  const dot = document.getElementById("ledbDot");
  if (dot) dot.className = "dot " + (data.state ? "yellow" : "gray");
}

// Mapa de nome de protocolo IR para índice numérico.
// Deve espelhar exatamente o enum IR_ReceptorMode em globals.h.
const irModeMap = {
  ALL: 0, KNOWN: 1, DISABLED: 2,
  NEC: 3, SONY: 4, RC5: 5, RC6: 6,
  SAMSUNG: 7, NIKAI: 8, LG: 9, JVC: 10, WHYNTER: 11,
};

// Atualiza estado do emissor e receptor IR.
function updateIRWS(data) {
  setText("irEmitter", data.emissor_teste ? "Ativo" : "Desligado");
  setText("irMode",    data.receptor_protocol || "--");

  if (data.receptor_protocol && irModeMap[data.receptor_protocol] !== undefined) {
    state.irModeIndex = irModeMap[data.receptor_protocol];
    const sel = document.getElementById("irReceptorSelect");
    if (sel) sel.value = irModeMap[data.receptor_protocol];
  }

  // Atualiza dot apenas se não há timer ativo (evita conflito com flash de sinal recebido).
  if (!state.irDotTimer) {
    const dot = document.getElementById("irDot");
    if (dot)
      dot.className =
        "dot " + (data.receptor_protocol && data.receptor_protocol !== "DISABLED" ? "green" : "yellow");
  }
}

// Atualiza dados do sensor AHT10.
function updateSensorWS(data) {
  const status = document.getElementById("sensorAHT10Status");
  const text   = document.getElementById("sensorAHT10Data");
  if (!status || !text) return;

  if (data.status) {
    status.className   = "status offline";
    status.textContent = data.status;
    text.textContent   = "";
    return;
  }

  text.innerHTML     = `Temperatura: ${data.temperatura} °C<br>Umidade: ${data.umidade} %`;
  status.className   = "status online";
  status.textContent = "online";
}

// Trata sinal IR recebido em tempo real: pisca dot e adiciona ao histórico.
function updateIRReceptorWS(data) {
  flashIRDot();
  saveIRToHistory({ ...data, timestamp: new Date().toLocaleTimeString("pt-BR") });
}

// Pisca o dot IR por 300ms ao receber um sinal.
function flashIRDot() {
  const dot = document.getElementById("irDot");
  if (!dot) return;

  if (state.irDotTimer) { clearTimeout(state.irDotTimer); state.irDotTimer = null; }

  dot.className = "dot green";
  state.irDotTimer = setTimeout(() => {
    const d = document.getElementById("irDot"); // re-busca após possível navegação SPA
    if (d) d.className = "dot yellow";
    state.irDotTimer = null;
  }, 300);
}

// Atualiza dados de rede.
function updateNetworkWS(data) {
  ["mdns", "wifi", "ip", "gateway", "mask", "rssi"].forEach((id) => setText(id, data[id]));
}

// Atualiza dados MQTT (status, server, tópicos, contadores).
function updateMQTTWS(data) {
  const cardMQTT = document.getElementById("cardMQTT");
  if (cardMQTT) cardMQTT.style.display = data.enabled ? "" : "none";

  setText("mqttServer",   data.server);
  setText("mqttPort",     data.port);
  setText("mqttClient",   data.client_id);
  setText("topic_main",   data.topic_main);
  setText("mqttSucessos", data.sucesso);
  setText("mqttErros",    data.erro);

  const status = document.getElementById("mqttStatus");
  if (!status) return;

  if (!data.enabled)    { status.className = "status warn";    status.textContent = "desabilitado"; }
  else if (data.status) { status.className = "status online";  status.textContent = "online"; }
  else                  { status.className = "status offline"; status.textContent = "offline"; }
}

// Reaplica o estado do receptor IR ao navegar entre páginas, sem acionar o flash do dot.
// Diferente de updateIRReceptorWS (que trata sinais ao vivo), esta função apenas
// restaura o select e o dot para o último estado conhecido.
function applyIRReceptorState(data) {
  if (data.receptor_protocol && irModeMap[data.receptor_protocol] !== undefined) {
    const sel = document.getElementById("irReceptorSelect");
    if (sel) sel.value = irModeMap[data.receptor_protocol];
  }
  if (!state.irDotTimer) {
    const dot = document.getElementById("irDot");
    if (dot)
      dot.className =
        "dot " + (data.receptor_protocol && data.receptor_protocol !== "DISABLED" ? "green" : "yellow");
  }
}

// Reaplica o último estado conhecido de cada type ao navegar entre páginas.
// ir_receptor usa applyIRReceptorState (restauração silenciosa) em vez de
// updateIRReceptorWS (que aciona flash e adiciona ao histórico).
function replayLastPayloads() {
  const updaters = {
    system:      updateSystemWS,
    ledb:        updateLEDBWS,
    sensor:      updateSensorWS,
    ir:          updateIRWS,
    ir_receptor: applyIRReceptorState,
    network:     updateNetworkWS,
    mqtt:        updateMQTTWS,
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
  const d   = Math.floor(s / 86400);
  const h   = Math.floor((s % 86400) / 3600);
  const m   = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  return `${d}d ${String(h).padStart(2, "0")}h ${String(m).padStart(2, "0")}m ${String(sec).padStart(2, "0")}s`;
}

function restartUptimeInterval() {
  if (state.uptimeInterval) clearInterval(state.uptimeInterval);
  state.uptimeInterval = setInterval(() => {
    state.uptimeSeconds++;
    setText("uptime", formatUptime(state.uptimeSeconds));
  }, 1000);
}

if (!state.uptimeInterval) restartUptimeInterval();

/* =========================================================
   10. HISTÓRICO IR
   Armazena os últimos 10 sinais recebidos.
   Click simples: copia o hex para a área de transferência.
   Double-click: carrega no form de envio manual.
========================================================= */

// Adiciona entrada ao histórico, ignorando duplicatas consecutivas.
function saveIRToHistory(payload) {
  if (payload.dec === undefined || payload.dec === null) return;
  if (String(state.irHistory[0]?.dec) === String(payload.dec)) return;
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
        navigator.clipboard
          .writeText(d.hex)
          .then(() => showIrToast("✅ Copiado!"))
          .catch(() => showIrToast("❌ Falha ao copiar", true));
      } else {
        // Fallback para browsers sem Clipboard API.
        const ta = document.createElement("textarea");
        ta.value = d.hex;
        ta.style.cssText = "position:fixed;opacity:0";
        document.body.appendChild(ta);
        ta.focus(); ta.select();
        document.execCommand("copy");
        document.body.removeChild(ta);
        showIrToast("✅ Copiado!");
      }
    };

    // Double-click: carrega no form de envio manual.
    li.ondblclick = () => {
      const proto = document.getElementById("irProto");
      const code  = document.getElementById("irCode");
      const bits  = document.getElementById("irBits");
      if (proto) proto.value = d.protocol;
      if (code)  { code.value = d.hex; code.placeholder = "Código hex (ex: 0x20DF10EF)"; }
      if (bits)  bits.value  = d.bits;
      li.classList.add("flash");
      setTimeout(() => li.classList.remove("flash"), 400);
    };

    list.appendChild(li);
  });
}

/* =========================================================
   11. HISTÓRICO DE LOGS (Console)
   Armazena as últimas 200 mensagens recebidas via WS "console".
   Renderização incremental: só acrescenta novas linhas ao DOM,
   sem re-renderizar o histórico completo a cada mensagem.
========================================================= */

// Adiciona mensagem ao histórico e re-renderiza incrementalmente.
function saveLogToHistory(data) {
  if (!data.msg) return;

  const timestamp    = new Date().toLocaleTimeString("pt-BR");
  const logTag       = data.log || "[LOG]";
  const color        = getLogColor(logTag);
  const safeMsg      = escapeHtml(data.msg);
  const tagFormatted = logTag.padEnd(10, " ");

  const line =
    `<span style="color:#888">${timestamp}</span> ` +
    `<span style="color:${color}; font-weight:bold">${tagFormatted}</span> ` +
    `<span style="color:${color}">- ${safeMsg}</span>`;

  state.logHistory.push(line);
  if (state.logHistory.length > 200)
    state.logHistory.splice(0, state.logHistory.length - 200);

  renderLogHistory();
}

// Renderiza incrementalmente o histórico de logs no DOM.
// Ao navegar para /system, logRenderedIndex é zerado para exibir o histórico completo.
function renderLogHistory() {
  const el = document.getElementById("log");
  if (!el) return;

  // Se o DOM foi limpo (navegação), começa do zero.
  if (el.childNodes.length === 0) state.logRenderedIndex = 0;

  const slice = state.logHistory.slice(state.logRenderedIndex);
  if (slice.length === 0) return;

  const frag = document.createDocumentFragment();
  slice.forEach((line) => {
    const span = document.createElement("span");
    span.innerHTML = line + "<br>";
    frag.appendChild(span);
  });
  el.appendChild(frag);
  state.logRenderedIndex = state.logHistory.length;

  // Limita o DOM a 200 nós para evitar crescimento ilimitado.
  while (el.childNodes.length > 200) el.removeChild(el.firstChild);
  el.scrollTop = el.scrollHeight;
}

// Limpa o histórico de logs.
function clearLogs() {
  state.logHistory = [];
  const el = document.getElementById("log");
  if (el) el.innerHTML = "";
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

// Retorna a cor associada à tag de log para colorização do console.
function getLogColor(tag) {
  if (!tag) return "#ffffff";
  tag = tag.toUpperCase();

  if (tag.includes("HTTP"))   return "#00FFFF"; // ciano
  if (tag.includes("MQTT"))   return "#FFA500"; // laranja
  if (tag.includes("WIFI"))   return "#8e5a8a"; // roxo
  if (tag.includes("WS"))     return "#60a5fa"; // azul claro
  if (tag.includes("IR"))     return "#a78bfa"; // violeta
  if (tag.includes("FS"))     return "#34d399"; // verde
  if (tag.includes("AHT"))    return "#22d3ee"; // ciano claro
  if (tag.includes("SYS"))    return "#f472b6"; // rosa
  if (tag.includes("BTN"))    return "#fb923c"; // laranja claro
  if (tag.includes("OTA"))    return "#facc15"; // amarelo
  if (tag.includes("LED A"))  return "#009dff"; // azul
  if (tag.includes("LED B"))  return "#FFD700"; // dourado
  if (tag.includes("ERROR"))  return "#FF4C4C"; // vermelho
  if (tag.includes("WARN"))   return "#FFD700"; // dourado
  if (tag.includes("INFO"))   return "#87CEFA"; // azul claro
  if (tag.includes("TELNET")) return "#94a3b8"; // cinza azulado
  if (tag.includes("AUTH"))   return "#f87171"; // vermelho claro
  if (tag.includes("TESTE"))  return "#c084fc"; // lilás
  return "#CCCCCC";
}

// Escapa caracteres HTML especiais para exibição segura no console.
function escapeHtml(str) {
  return str.replace(/[&<>"']/g, (m) =>
    ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[m])
  );
}

/* =========================================================
   12. GERENCIAMENTO DE ARQUIVOS
   Upload via POST /upload com autenticação HTTP Basic.
   Download via GET /download?file=<path>.
   As funções exportConfig/importConfig/exportRemotes/importRemotes
   delegam para os helpers genéricos downloadFile e uploadFile.
========================================================= */

// Retorna o header Authorization Basic.
// Usa o admin_user da última config recebida, com fallback para "admin".
function makeBasicAuth(pass) {
  const user = state.lastConfig?.admin_user || "admin";
  return "Basic " + btoa(`${user}:${pass}`);
}

// Faz download de um arquivo do LittleFS e dispara o save no browser.
// statusFn(msg, color) é chamado para feedback ao usuário.
async function downloadFile(path, filename, statusFn) {
  try {
    const res = await fetch(`/download?file=${path}`);
    if (res.status === 401) { statusFn("❌ Sem permissão.", "#ef4444"); return; }
    if (!res.ok)            { statusFn("❌ Arquivo não encontrado.", "#ef4444"); return; }
    const a  = document.createElement("a");
    a.href     = URL.createObjectURL(await res.blob());
    a.download = filename;
    a.click();
    URL.revokeObjectURL(a.href);
  } catch {
    statusFn("❌ Erro ao exportar.", "#ef4444");
  }
}

// Faz upload de um arquivo para o LittleFS via POST /upload.
// Valida JSON antes de enviar se o arquivo for .json.
// statusFn(msg, color) é chamado para feedback; onSuccess é chamado após upload ok.
async function uploadFile(file, statusFn, onSuccess = null) {
  if (!file) { alert("Selecione um arquivo."); return; }

  // Valida JSON antes de enviar para evitar corromper o filesystem.
  if (file.name.endsWith(".json")) {
    try { JSON.parse(await file.text()); }
    catch { statusFn("❌ Arquivo JSON inválido.", "#ef4444"); return; }
  }

  const httpPass = prompt("🔐 Senha para continuar:");
  if (!httpPass) return;

  const formData = new FormData();
  formData.append("upload", file);

  const xhr = new XMLHttpRequest();
  xhr.withCredentials = true;
  xhr.open("POST", "/upload", true);
  xhr.setRequestHeader("Authorization", makeBasicAuth(httpPass));

  xhr.onload = () => {
    if (xhr.status === 401) { statusFn("❌ Senha incorreta.", "#ef4444"); return; }
    statusFn("✅ Importado com sucesso!", "#22c55e");
    if (onSuccess) onSuccess();
  };
  xhr.onerror = () => statusFn("❌ Erro no upload.", "#ef4444");
  xhr.send(formData);
}

// --- config.json ---

async function exportConfig() {
  await downloadFile("/config.json", "config.json", (msg, color) =>
    showStatus("configFileStatus", msg, color)
  );
}

async function importConfig() {
  if (!confirm("⚠️ O config.json contém dados sensíveis. Deseja continuar?")) return;
  const file = document.getElementById("configFile")?.files[0];
  await uploadFile(file, (msg, color) => showStatus("configFileStatus", msg, color));
}

// --- remotes.json ---

async function exportRemotes() {
  await downloadFile("/remotes.json", "remotes.json", (msg, color) =>
    showStatus("remotesStatus", msg, color)
  );
}

async function importRemotes() {
  const file = document.getElementById("remotesFile")?.files[0];
  if (file && file.name !== "remotes.json") {
    showStatus("remotesStatus", "❌ O arquivo deve se chamar remotes.json", "#ef4444");
    return;
  }
  await uploadFile(
    file,
    (msg, color) => showStatus("remotesStatus", msg, color),
    loadRemotes // recarrega os controles após importar com sucesso
  );
}

// --- OTA ---

// Exibe mensagem de status do OTA sem auto-limpeza (exige ação do usuário).
function showOtaStatus(msg, color) {
  showStatus("otaStatus", msg, color, 0);
}

async function startOTAUpdate() {
  const file = document.getElementById("otaFile")?.files[0];
  if (!file) return alert("Selecione um arquivo .bin");

  if (!file.name.endsWith(".bin")) {
    showOtaStatus("❌ Arquivo inválido. Selecione um .bin", "#ef4444");
    return;
  }

  if (!confirm(`Atualizar firmware com "${file.name}"? O dispositivo será reiniciado.`)) return;

  const httpPass = prompt("🔐 Senha para continuar:");
  if (!httpPass) return;

  const formData = new FormData();
  formData.append("update", file);

  const progress = document.getElementById("otaProgress");
  const xhr      = new XMLHttpRequest();

  xhr.upload.onprogress = (e) => {
    if (!progress) return;
    progress.style.display = "block";
    if (e.lengthComputable) progress.value = (e.loaded / e.total) * 100;
  };

  xhr.onload = () => {
    if (progress) progress.style.display = "none";
    if      (xhr.status === 200) showOtaStatus("✅ Firmware enviado! Aguarde o reboot...", "#22c55e");
    else if (xhr.status === 401) showOtaStatus("❌ Senha incorreta.", "#ef4444");
    else                          showOtaStatus(`❌ Erro: HTTP ${xhr.status}`, "#ef4444");
  };

  xhr.onerror = () => {
    if (progress) progress.style.display = "none";
    showOtaStatus("❌ Falha na conexão.", "#ef4444");
  };

  showOtaStatus("⏳ Enviando firmware...", "#facc15");
  xhr.open("POST", "/update", true);
  xhr.setRequestHeader("Authorization", makeBasicAuth(httpPass));
  xhr.send(formData);
}

/* =========================================================
   13. CONTROLES DE REDE (settings)
========================================================= */

// Exibe/oculta campos de IP fixo conforme o modo selecionado.
function toggleIPFields() {
  const mode   = document.getElementById("cfg_ip_mode")?.value;
  const fields = document.getElementById("cfg_ip_fields");
  if (fields) fields.style.display = mode === "static" ? "block" : "none";
}

// Exibe/oculta campos MQTT conforme habilitado/desabilitado.
function toggleMQTTFields() {
  const enabled = document.getElementById("cfg_mqtt_enabled")?.value;
  const fields  = document.getElementById("cfg_mqtt_fields");
  if (fields) fields.style.display = enabled === "true" ? "block" : "none";
}

// Valida formato de endereço IP. String vazia é aceita (indica modo DHCP).
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
   O JSON suporta: type "button", "space", "label" e campos
   opcionais: span, rowSpan, background, color, fontSize.
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
    showStatus("remotesStatus", "❌ Erro ao carregar remotes.json. Faça o upload do arquivo.", "#ef4444");
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
    opt.value = opt.textContent = model;
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
  if (!state.remotesData?.[model]) {
    console.warn(`Dados para o modelo ${model} ainda não disponíveis.`);
    return;
  }

  const container = document.getElementById("buttonsContainer");
  if (!container) return;

  container.innerHTML = "";

  const buttons = state.remotesData[model].buttons || state.remotesData[model].button || [];

  buttons.forEach((btn) => {
    // Espaço vazio no grid.
    if (btn.type === "space") {
      const space = document.createElement("div");
      if (btn.span) space.className = `span-${btn.span}`;
      container.appendChild(space);
      return;
    }

    // Label descritivo (sem ação).
    if (btn.type === "label") {
      const label       = document.createElement("div");
      label.textContent = btn.name || "";
      label.className   = `remote-label${btn.span ? ` span-${btn.span}` : " span-3"}`;
      container.appendChild(label);
      return;
    }

    // Ignora tipos desconhecidos (exceto "button" e sem type).
    if (btn.type && btn.type !== "button") return;

    const b       = document.createElement("button");
    b.textContent = btn.name;
    b.type        = "button";
    b.className   = "btn-ir-remote";

    if (btn.fontSize)   b.style.fontSize   = btn.fontSize;
    if (btn.span)       b.classList.add(`span-${btn.span}`);
    if (btn.rowSpan)    b.classList.add(`row-span-${btn.rowSpan}`);
    if (btn.background) b.style.background = `#${btn.background}`;
    if (btn.color)      b.style.color      = `#${btn.color}`;

    b.onclick = () => {
      if (!btn.code) { showIrToast("Botão sem código IR", true); return; }
      sendIR(btn.protocol, btn.code, btn.bits, b);
    };

    container.appendChild(b);
  });
}

/* =========================================================
   15. IR — ENVIO
========================================================= */

// Envia código IR de um botão do controle remoto.
// O código é passado como string para preservar o prefixo 0x e evitar
// perda de precisão em inteiros de 64 bits no JSON.
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
  wsSend({ cmd: "sendIR", code: String(code), protocol, bits: parseInt(bits) || 32 });
}

// Exibe toast de feedback de envio IR (roxo = ok, vermelho = erro).
function showIrToast(message, isError = false) {
  document.querySelector(".ir-sending-toast")?.remove();
  const toast       = document.createElement("div");
  toast.className   = "ir-sending-toast" + (isError ? " error" : "");
  toast.textContent = message;
  document.body.appendChild(toast);
  setTimeout(() => toast.remove(), 1000);
}

// Envia código IR digitado manualmente no form.
// Hex sem prefixo 0x é auto-prefixado antes do envio para garantir
// interpretação correta pelo backend (strtoull com base 0).
function sendIRManual() {
  const code  = document.getElementById("irCode")?.value.trim();
  const proto = document.getElementById("irProto")?.value;
  const bits  = parseInt(document.getElementById("irBits")?.value) || 32;

  if (!code) return alert("Digite um código IR.");

  const isHex0x  = /^0x[0-9a-fA-F]+$/i.test(code);
  const isHexRaw = /^[0-9a-fA-F]*[a-fA-F][0-9a-fA-F]*$/.test(code);
  const isDec    = /^\d+$/.test(code);

  if (!isHex0x && !isHexRaw && !isDec)
    return alert("Código inválido. Use hex (0x20DF10EF) ou decimal (551489775).");

  const finalCode = isHexRaw ? "0x" + code : code;
  wsSend({ cmd: "sendIR", code: finalCode, protocol: proto, bits });
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
   Todos os comandos destrutivos pedem confirmação antes de enviar.
========================================================= */

function rebootDevice() {
  if (!confirm("Reiniciar o dispositivo?")) return;
  wsSend({ cmd: "reboot" });
}

function openWifiPortal() {
  if (!confirm("Abrir portal de configuração WiFi?")) return;
  wsSend({ cmd: "wifiPortal" });
}

function resetWifi() {
  if (!confirm("Resetar configurações WiFi?")) return;
  wsSend({ cmd: "resetWifi" });
}

function resetConfig() {
  if (!confirm("Apagar config.json? Isso apagará todas as configurações!")) return;
  wsSend({ cmd: "resetConfig" });
}

/* =========================================================
   19. CONFIGURAÇÃO DO DISPOSITIVO
========================================================= */

// Popula o form de configurações com os dados recebidos do backend.
// Chamado uma vez por conexão WS (via updateSystemWS) ou ao navegar
// para /settings se state.lastConfig estiver disponível.
function populateConfig(data) {
  const fields = {
    cfg_hostname:      data.hostname,
    cfg_mqtt_id:       data.mqtt_id,
    cfg_grupo:         data.grupo,
    cfg_ip:            data.ip,
    cfg_gw:            data.gw,
    cfg_sn:            data.sn,
    cfg_mqtt_server:   data.mqtt_server,
    cfg_mqtt_port:     data.mqtt_port,
    cfg_mqtt_user:     data.mqtt_user,
    cfg_mqtt_enabled:  data.mqtt_enabled  ? "true" : "false",
    cfg_aht10_enabled: data.aht10_enabled ? "true" : "false",
    cfg_wifi_ssid:     data.wifi_ssid,
    cfg_admin_user:    data.admin_user,
    // cfg_mqtt_password e cfg_ws_password omitidos — backend não envia senhas
  };

  Object.entries(fields).forEach(([id, val]) => {
    const el = document.getElementById(id);
    if (el && val !== undefined) el.value = val;
  });

  // Detecta modo IP (DHCP ou fixo) e exibe campos correspondentes.
  const modeEl = document.getElementById("cfg_ip_mode");
  if (modeEl) {
    modeEl.value = data.ip && data.ip !== "" && data.ip !== "0.0.0.0" ? "static" : "dhcp";
    toggleIPFields();
  }

  // Sincroniza visibilidade dos campos MQTT.
  if (document.getElementById("cfg_mqtt_enabled")) toggleMQTTFields();
}

// Valida e envia o payload de configuração para o backend via WS.
// Senhas em branco são enviadas como "__keep__" para que o
// backend preserve o valor atual sem sobrescrever.
function saveDeviceConfig() {
  state._settingsFormDirty = false;
  const get = (id) => document.getElementById(id)?.value ?? "";

  const ipMode = document.getElementById("cfg_ip_mode")?.value;
  const ip     = ipMode === "static" ? get("cfg_ip").trim() : "";
  const gw     = ipMode === "static" ? get("cfg_gw").trim() : "";
  const sn     = ipMode === "static" ? get("cfg_sn").trim() : "";

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

  const wsPassword   = get("cfg_ws_password");
  const wifiPassword = get("cfg_wifi_password");
  const mqttPassword = get("cfg_mqtt_password");
  const adminUser    = get("cfg_admin_user");

  wsSend({
    cmd:           "saveConfig",
    hostname:      get("cfg_hostname").trim(),
    mqtt_id:       get("cfg_mqtt_id").trim(),
    grupo:         get("cfg_grupo").trim(),
    wifi_ssid:     get("cfg_wifi_ssid").trim(),
    wifi_password: wifiPassword.length > 0 ? wifiPassword : "__keep__",
    ip, gw, sn,
    mqtt_server:   get("cfg_mqtt_server").trim(),
    mqtt_user:     get("cfg_mqtt_user").trim(),
    mqtt_password: mqttPassword.length > 0 ? mqttPassword : "__keep__",
    mqtt_enabled:  get("cfg_mqtt_enabled") === "true",
    mqtt_port:     parseInt(get("cfg_mqtt_port")) || 1883,
    aht10_enabled: get("cfg_aht10_enabled") === "true",
    ws_password:   wsPassword.length > 0 ? wsPassword : "__keep__",
    admin_user:    adminUser.length  > 0 ? adminUser  : "__keep__",
    ir_receptor:   state.irModeIndex,
  });

  showCfgStatus("⏳ Salvando...", "#facc15");

  // Timeout de confirmação — avisa se não receber "configSaved" em 5s.
  if (state.saveConfigTimer) clearTimeout(state.saveConfigTimer);
  state.saveConfigTimer = setTimeout(() => {
    state.saveConfigTimer = null;
    showCfgStatus("⚠️ Sem confirmação do dispositivo. Verifique a conexão.", "#f59e0b");
  }, 5000);
}

// Exibe mensagem de status no form de configurações por 3 segundos.
// Ao confirmar sucesso, remove o indicador de campo modificado de todos os inputs.
function showCfgStatus(msg, color) {
  showStatus("cfgStatus", msg, color);
  if (color === "#22c55e") {
    document.querySelectorAll(".field-dirty").forEach((f) => f.classList.remove("field-dirty"));
  }
}

/* =========================================================
   20. INICIALIZAÇÃO
   Executado uma vez ao carregar a página.
========================================================= */

document.addEventListener("DOMContentLoaded", () => {
  // Cancela timer de dot que pode ter sobrado de navegação anterior.
  if (state.irDotTimer) { clearTimeout(state.irDotTimer); state.irDotTimer = null; }

  setText("uptime", "0d 00h 00m 00s");
  renderIRHistory();
  renderLogHistory();

  const currentPath = window.location.pathname;
  navigateTo(routes[currentPath] ? currentPath : "/");

  updateWSStatus(false);
  connectWS();

  // Se WS já estava aberto (navegação SPA sem reload), atualiza badge imediatamente.
  if (state.ws && state.ws.readyState === WebSocket.OPEN) updateWSStatus(true);
});