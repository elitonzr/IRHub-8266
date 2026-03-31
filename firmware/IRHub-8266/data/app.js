/* =========================================================
   STATE
========================================================= */
let ws = null;
let reconnectTimer = null;
let wsQueue = [];
let uptimeSeconds = 0;
let irHistory = [];
let configPopulated = false;

/* =========================================================
   WEBSOCKET
========================================================= */
function connectWS() {
  if (ws && ws.readyState === WebSocket.OPEN) return;
  ws = new WebSocket(`ws://${location.hostname}:81`);

  ws.onopen = () => {
    updateWSStatus(true);
    flushQueue();
    if (reconnectTimer) { clearTimeout(reconnectTimer); reconnectTimer = null; }
  };

  ws.onclose = () => {
    updateWSStatus(false);
    configPopulated = false;
    scheduleReconnect();
  };

  ws.onerror = () => ws.close();
  ws.onmessage = handleWSMessage;
}

function scheduleReconnect() {
  if (reconnectTimer) return;
  reconnectTimer = setTimeout(() => { reconnectTimer = null; connectWS(); }, 2000);
}

function wsSend(msg) {
  const data = typeof msg === 'object' ? JSON.stringify(msg) : msg;
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(data);
  } else {
    wsQueue.push(data);
  }
}

function flushQueue() {
  while (wsQueue.length && ws.readyState === WebSocket.OPEN) {
    ws.send(wsQueue.shift());
  }
}

/* =========================================================
   MESSAGE HANDLER
========================================================= */
function handleWSMessage(event) {
  let data;
  try { data = JSON.parse(event.data); } catch { return; }
  if (!data.type) return;

  switch (data.type) {
    case 'system':      updateSystemWS(data);     break;
    case 'outputs':     updateOutputsWS(data);    break;
    case 'sensor':      updateSensorWS(data);     break;
    case 'ir':          updateIRWS(data);          break;
    case 'ir_receptor': updateIRReceptorWS(data); break;
    case 'network':     updateNetworkWS(data);    break;
    case 'mqtt':        updateMQTTWS(data);       break;
    case 'reboot':      alert('Dispositivo reiniciando...'); break;
    case 'wifiPortal':  alert("Conecte-se à rede '" + (document.getElementById('name').textContent || 'irhub8266') + "'"); break;
    case 'wifiReset':   alert('WiFi resetado!'); break;
    case 'configSaved': showCfgStatus('✅ Configuração salva!', '#22c55e'); break;
    case 'configError': showCfgStatus('❌ Erro: ' + (data.msg || ''), '#ef4444'); break;
    case 'configReset': alert('Configurações resetadas!'); break;
  }
}

/* =========================================================
   UI UPDATERS
========================================================= */
function updateWSStatus(connected) {
  const el = document.getElementById('wsStatus');
  el.innerHTML = connected
    ? '<span class="status online">⚡ Conectado</span>'
    : '<span class="status offline">⚡ WebSocket desconectado</span>';
  const name = document.getElementById('name').textContent;
  document.title = connected ? `✅ ${name}` : `❌ ${name}`;
}

function updateSystemWS(data) {
  setText('name',          data.name);
  setText('buildDateTime', data.buildDateTime);
  setText('buildVersion',  data.buildVersion);
  setText('heap',          data.heap);
  if (data.uptime_seconds !== undefined) uptimeSeconds = data.uptime_seconds;

  if (data.config && !configPopulated) {
    populateConfig(data.config);
    configPopulated = true;
  }
}

function updateOutputsWS(data) {
  setText('ledState', data.led ? 'Ligado' : 'Desligado');
  document.getElementById('ledDot').className = 'dot ' + (data.led ? 'green' : 'red');
}

function updateIRWS(data) {
  setText('irEmitter', data.emissor_teste ? 'Ativo' : 'Desligado');
  setText('irMode',    data.receptor_protocolo || '--');
}

function updateSensorWS(data) {
  const status = document.getElementById('sensorAHT10Status');
  const text   = document.getElementById('sensorAHT10Data');

  if (data.status && data.status !== 'ONLINE') {
    status.className = 'status offline';
    status.textContent = data.status;
    text.textContent = '';
    return;
  }
  text.innerHTML = `Temperatura: ${data.temperatura} °C<br>Umidade: ${data.umidade} %`;
  status.className = 'status online';
  status.textContent = 'online';
}

function updateIRReceptorWS(data) {
  const dot = document.getElementById('irDot');
  dot.className = 'dot green';
  setTimeout(() => dot.className = 'dot yellow', 300);
  saveIRToHistory({ timestamp: new Date().toLocaleTimeString(), ...data });
}

function updateNetworkWS(data) {
  ['mdns', 'wifi', 'ip', 'gateway', 'mask', 'rssi'].forEach(id => setText(id, data[id]));
}

function updateMQTTWS(data) {
  setText('mqttServer',   data.server);
  setText('mqttClient',   data.client_id);
  setText('topic_main',   data.topic_main);
  setText('mqttSucessos', data.sucesso);
  setText('mqttErros',    data.erro);

  const status = document.getElementById('mqttStatus');
  if (!data.enabled) {
    status.className = 'status warn';   status.textContent = 'desabilitado';
  } else if (data.status) {
    status.className = 'status online'; status.textContent = 'online';
  } else {
    status.className = 'status offline'; status.textContent = 'offline';
  }
}

/* helper */
function setText(id, val) {
  const el = document.getElementById(id);
  if (el && val !== undefined && val !== null) el.textContent = val;
}

/* =========================================================
   UPTIME
========================================================= */
function formatUptime(s) {
  const d   = Math.floor(s / 86400);
  const h   = Math.floor((s % 86400) / 3600);
  const m   = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  return `${d}d ${String(h).padStart(2,'0')}h ${String(m).padStart(2,'0')}m ${String(sec).padStart(2,'0')}s`;
}

setInterval(() => {
  uptimeSeconds++;
  setText('uptime', formatUptime(uptimeSeconds));
}, 1000);

/* =========================================================
   IR HISTORY
========================================================= */
function saveIRToHistory(payload) {
  if (irHistory[0]?.dec === payload.dec) return;
  irHistory.unshift(payload);
  if (irHistory.length > 10) irHistory.pop();
  renderIRHistory();
}

function cleanHistory() {
  irHistory = [];
  renderIRHistory();
}

function renderIRHistory() {
  const list = document.getElementById('irHistory');
  if (!list) return;
  list.innerHTML = '';

  irHistory.forEach(d => {
    const li = document.createElement('li');
    li.textContent = `${d.timestamp} | ${d.protocolo} | ${d.bits}b | ${d.dec} | ${d.hex} 📋`;

    // clique simples = copiar hex
    li.onclick = () => navigator.clipboard.writeText(d.hex).catch(() => {});

    // duplo clique = carregar no formulário
    li.ondblclick = () => {
      const proto  = document.getElementById('irProto');
      const format = document.getElementById('irFormat');
      const code   = document.getElementById('irCode');
      const bits   = document.getElementById('irBits');
      if (proto)  proto.value  = d.protocolo;
      if (format) format.value = 'hex';
      if (code)   { code.value = d.hex; code.placeholder = 'Código hex (ex: 0x20DF10EF)'; }
      if (bits)   bits.value   = d.bits;
      li.classList.add('flash');
      setTimeout(() => li.classList.remove('flash'), 400);
    };

    list.appendChild(li);
  });
}

/* =========================================================
   CONFIG
========================================================= */
function populateConfig(data) {
  const fields = {
    cfg_hostname:     data.hostname,
    cfg_mqtt_id:      data.mqtt_id,
    cfg_grupo:        data.grupo,
    cfg_ip:           data.ip,
    cfg_gw:           data.gw,
    cfg_sn:           data.sn,
    cfg_mqtt_server:  data.mqtt_server,
    cfg_mqtt_user:    data.mqtt_user,
    cfg_mqtt_enabled: data.mqtt_enabled,
    // cfg_mqtt_password intencionalmente omitido — servidor não envia a senha
  };
  Object.entries(fields).forEach(([id, val]) => {
    const el = document.getElementById(id);
    if (el && val !== undefined) el.value = val;
  });
}

function saveDeviceConfig() {
  const get = id => document.getElementById(id)?.value ?? '';
  const password = get('cfg_mqtt_password');

  const payload = {
    cmd:           'saveConfig',
    hostname:      get('cfg_hostname').trim(),
    mqtt_id:       get('cfg_mqtt_id').trim(),
    grupo:         get('cfg_grupo').trim(),
    ip:            get('cfg_ip').trim(),
    gw:            get('cfg_gw').trim(),
    sn:            get('cfg_sn').trim(),
    mqtt_server:   get('cfg_mqtt_server').trim(),
    mqtt_user:     get('cfg_mqtt_user').trim(),
    // campo vazio = manter senha atual no ESP
    mqtt_password: password.length > 0 ? password : '__keep__',
    mqtt_enabled:  get('cfg_mqtt_enabled'),
  };

  wsSend(payload);
  showCfgStatus('⏳ Salvando...', '#facc15');
}

function showCfgStatus(msg, color) {
  const el = document.getElementById('cfgStatus');
  if (!el) return;
  el.textContent = msg;
  el.style.color = color;
  setTimeout(() => el.textContent = '', 3000);
}

/* =========================================================
   COMMANDS
========================================================= */
function sendIRManual() {
  const code   = document.getElementById('irCode').value.trim();
  const proto  = document.getElementById('irProto').value;
  const bits   = parseInt(document.getElementById('irBits').value) || 32;
  const format = document.getElementById('irFormat').value;

  if (!code) return alert('Digite um código IR.');

  let hex;
  if (format === 'dec') {
    const num = parseInt(code, 10);
    if (isNaN(num)) return alert('Decimal inválido.');
    hex = '0x' + num.toString(16).toUpperCase();
  } else {
    hex = code;
  }

  wsSend({ cmd: 'sendIR', hex, protocolo: proto, bits });
}

function toggleLED()       { wsSend('toggleLED'); }
function toggleIREmissor() { wsSend('toggleIREmissor'); }

const IR_MODOS = ['DESABILITADO', 'NEC', 'NIKAI', 'NEC e NIKAI', 'TUDO'];
function toggleIRReceptor() {
  const atual = document.getElementById('irMode').textContent.trim();
  const idx   = IR_MODOS.indexOf(atual);
  const prox  = (idx + 1) % IR_MODOS.length;
  wsSend({ cmd: 'setIRReceptor', mode: prox });
}

function rebootDevice()   { if (confirm('Reiniciar?'))                                    wsSend({ cmd: 'reboot' }); }
function openWifiPortal() { if (confirm('Abrir portal WiFi?'))                            wsSend({ cmd: 'wifiPortal' }); }
function resetWifi()      { if (confirm('Resetar WiFi?'))                                 wsSend({ cmd: 'wifiReset' }); }
function resetConfig()    { if (confirm('Reset total? Isso apagará todas as configurações.')) wsSend({ cmd: 'configReset' }); }

/* =========================================================
   INIT
========================================================= */
document.addEventListener('DOMContentLoaded', () => {
  setText('uptime', '0d 00h 00m 00s');

  const irFormat = document.getElementById('irFormat');
  if (irFormat) {
    irFormat.addEventListener('change', function () {
      const input = document.getElementById('irCode');
      if (!input) return;
      input.placeholder = this.value === 'dec'
        ? 'Código decimal (ex: 551489775)'
        : 'Código hex (ex: 0x20DF10EF)';
      input.value = '';
    });
  }

  connectWS();
});
