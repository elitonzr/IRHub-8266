#pragma once

const char PAGE_PORTAL_1[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>IRHub-8266 Setup</title>
<style>
body { font-family:sans-serif; background:#111827; color:#f9fafb; padding:20px; }
.card { background:#1f2937; padding:20px; border-radius:10px; max-width:400px; margin:auto; }

input, select, button { width:100%; padding:10px; margin-top:10px; border-radius:6px; box-sizing:border-box; }
input, select { background:#111827; color:#f9fafb; border:1px solid #374151; }
input:focus, select:focus { outline:none; border-color:#2563eb; }

button { background:#2563eb; color:white; border:none; cursor:pointer; border-radius:6px; font-weight:600; }
button:hover { background:#1d4ed8; }
button[style*='dc2626'] { background:#dc2626 !important; }
button[style*='dc2626']:hover { background:#b91c1c !important; }

</style>
</head>
<body>

<div class='card'>
<div style='text-align:center;margin-bottom:16px'>
  <img src='/logo.png' alt='IRHub' style='width:56px;height:56px;border-radius:50%;object-fit:cover;' onerror='this.style.display="none"'>
  <h2 style='margin:8px 0 0'>IRHub-8266</h2>
  <p style='margin:4px 0 0;opacity:0.5;font-size:13px'>Configuração WiFi</p>
</div>

<form action='/save' method='POST'>

<label>Redes disponíveis</label>
<div id='net_list' style='max-height:160px;overflow-y:auto;background:#111827;border:1px solid #374151;border-radius:6px;margin-top:10px'></div>

<label style='margin-top:10px'>SSID</label>
<input type='text' id='ssid_input' name='ssid_manual' placeholder='Selecione ou digite o SSID'>
<script>
var _nets=
)rawliteral";

const char PAGE_PORTAL_1B[] PROGMEM = R"rawliteral(;
(function(){
  var ul=document.getElementById('net_list');
  _nets.forEach(function(n){
    var d=document.createElement('div');
    d.textContent=n.ssid+' ('+n.rssi+' dBm)';
    d.style.cssText='padding:8px 10px;cursor:pointer;border-bottom:1px solid #374151;font-size:13px';
    d.onmouseover=function(){this.style.background='#1e293b'};
    d.onmouseout=function(){this.style.background=''};
    d.onclick=function(){document.getElementById('ssid_input').value=n.ssid};
    ul.appendChild(d);
  });
})();
</script>
)rawliteral";

const char PAGE_PORTAL_2[] PROGMEM = R"rawliteral(
<label>Senha WiFi</label>
<div style='display:flex;gap:8px;align-items:center;margin-top:10px'>
<input type='password' id='wifi_pass' name='wifi_pass' style='margin-top:0'>
<button type='button' onclick="togglePass('wifi_pass',this)" style='width:auto;padding:10px;margin-top:0'>👁</button>
</div>

<label>Senha Web (PasswordWS)</label>
<div style='display:flex;gap:8px;align-items:center;margin-top:10px'>
<input type='password' id='ws_pass' name='ws_pass' placeholder='Opcional' value=')rawliteral";

const char PAGE_PORTAL_2D[] PROGMEM = R"rawliteral(' style='margin-top:0'>
<button type='button' onclick="togglePass('ws_pass',this)" style='width:auto;padding:10px;margin-top:0'>👁</button>
</div>

<label>Grupo MQTT</label>
<input type='text' name='grupo' value=')rawliteral";

const char PAGE_PORTAL_2C[] PROGMEM = R"rawliteral('>

<label>Usuário Admin</label>
<input type='text' name='admin_user' value=')rawliteral";

const char PAGE_PORTAL_2E[] PROGMEM = R"rawliteral('>

<label>Hostname</label>
<input type='text' name='hostname' value=")rawliteral";

const char PAGE_PORTAL_3[] PROGMEM = R"rawliteral(">

<label>Modo de IP</label>
<div style='display:flex;gap:20px;margin-top:10px'>
  <label style='display:flex;align-items:center;gap:6px;cursor:pointer'>
    <input type='radio' name='ip_mode' id='ip_dhcp' value='dhcp' onchange="toggleIP(this.value)"> DHCP
  </label>
  <label style='display:flex;align-items:center;gap:6px;cursor:pointer'>
    <input type='radio' name='ip_mode' id='ip_static' value='static' onchange="toggleIP(this.value)"> IP Fixo
  </label>
</div>

<div id='ip_fields' style='display:none'>
  <label>IP</label>
  <input type='text' name='ip' placeholder='192.168.1.100' value=')rawliteral";

const char PAGE_PORTAL_4[] PROGMEM = R"rawliteral('>
  <label>Gateway</label>
  <input type='text' name='gw' placeholder='192.168.1.1' value=')rawliteral";

const char PAGE_PORTAL_5[] PROGMEM = R"rawliteral('>
  <label>Subnet</label>
  <input type='text' name='sn' placeholder='255.255.255.0' value=')rawliteral";

const char PAGE_PORTAL_6[] PROGMEM = R"rawliteral('>
</div>

<script>
function togglePass(id,btn){
  var el=document.getElementById(id);
  var show=el.type==='password';
  el.type=show?'text':'password';
  btn.textContent=show?'🙈':'👁';
}
</script>

<button type='submit'>Salvar e Reiniciar</button>
</form>

<hr style='border-color:#374151;margin:20px 0'>

<a href='/files' style='display:block;text-align:center;color:#60a5fa;margin-bottom:12px'>📁 File Manager</a>

<form action='/reboot' method='POST'>
  <button type='submit' style='background:#dc2626'>🔄 Reiniciar dispositivo</button>
</form>

</div>
</body>
</html>
)rawliteral";

// ==============================
// FILES_PAGE — upload
// ==============================
const char FILES_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>LittleFS Manager</title>
<script>
(function(){
  var t = localStorage.getItem('theme') || 'dark';
  document.documentElement.setAttribute('data-theme', t);
})();
</script>
<link rel="stylesheet" href="/style.css" />
</head>
<body>

<nav class="navbar">
  <img src="/logo.png" alt="IRHub" class="navbar-logo" />
  <button class="navbar-burger" onclick="drawerOpen()" aria-label="Menu">&#9776;</button>
  <span class="navbar-brand" id="name">IRHub-8266</span>
  <div class="navbar-links">
    <a href="/">Home</a>
    <a href="/system">System</a>
  </div>
  <button id="btnTheme" class="btn-navbar" onclick="toggleTheme()" title="Alternar tema (claro/escuro)" aria-label="Alternar tema">🌙</button>
</nav>

<div class="drawer-overlay" id="drawerOverlay" onclick="drawerClose()"></div>

<div class="drawer" id="drawer">
  <div class="drawer-header">
    <div class="drawer-title">IRHub-8266</div>
  </div>
  <span class="drawer-section-label">Navegação</span>
  <a href="/"><span class="drawer-icon">🏠</span> Home</a>
  <a href="/system"><span class="drawer-icon">🤖</span> System</a>
  <a href="/settings"><span class="drawer-icon">⚙️</span> Settings</a>
  <span class="drawer-section-label">Ferramentas</span>
  <a href="/files"><span class="drawer-icon">📁</span> File Manager</a>
  <span class="drawer-section-label">Preferências</span>
  <a href="#" onclick="toggleTheme(); drawerClose(); return false;">
    <span class="drawer-icon" id="drawerThemeIcon">🌙</span> Alternar Tema
  </a>
  <span class="drawer-section-label">Login</span>
  <button id="btnLogin" class="btn-navbar" onclick="handleLoginBtn()">
  Login
  </button>

        <div class="drawer-footer">
        <a
          href="https://github.com/elitonzr/IRHub-8266"
          target="_blank"
          rel="noopener"
        >
          <span class="drawer-icon">🐙</span> GitHub do Projeto
        </a>
      </div>
      
</div>

<div class="page-content">


  <h2>📁 LittleFS File Manager</h2>

<div class="card">
  <h3>📤 Upload de arquivo</h3>

  <div class="upload-row">
    <input type="file" id="file">
    <button class="btn-send" onclick="upload()">📤 Enviar</button>
  </div>

  <progress id="prog" value="0" max="100" style="display:none"></progress>
</div>

  <div class="card">
    <table>
      <tr>
        <th>📄 Arquivo</th>
        <th>📦 Tamanho</th>
        <th>⚙️ Ações</th>
      </tr>
      %FILES%
    </table>
  </div>

  <p><b>Uso:</b> %USAGE%</p>

</div>

<div id="appModal">
  <div class="app-modal-box">
    <p id="appModalMsg" class="app-modal-msg"></p>
    <div id="appModalInput" class="app-modal-input-wrap" style="display:none">
      <input id="appModalField" type="password" placeholder="Senha" />
    </div>
    <div class="app-modal-actions">
      <button id="appModalCancel" class="app-modal-btn app-modal-btn-cancel">Cancelar</button>
      <button id="appModalConfirm" class="app-modal-btn app-modal-btn-confirm">Confirmar</button>
    </div>
  </div>
</div>

<script src="/app.js"></script>

<script>
function upload(){
  const file=document.getElementById('file').files[0];
  if(!file){alert('Selecione um arquivo');return;}

  const xhr=new XMLHttpRequest();

  xhr.upload.onprogress=function(e){
  var prog=document.getElementById('prog');
  prog.style.display='block';
  if(e.lengthComputable) prog.value=(e.loaded/e.total)*100;
};

xhr.onload=function(){
  document.getElementById('prog').style.display='none';
  alert('Upload concluído');
  window.location.reload();
};

  const formData=new FormData();
  formData.append('upload',file);

appModal('🔐 Informe a senha para continuar:', {type:'prompt', confirmText:'Enviar', inputPlaceholder:'Senha HTTP'}).then(function(pass){
  if (!pass) return;
  xhr.withCredentials=true;
  xhr.open('POST','/upload',true);
  xhr.setRequestHeader('Authorization', 'Basic ' + btoa('%ADMIN_USER%:' + pass));
  xhr.send(formData);
});

}
</script>

</body>
</html>
)rawliteral";