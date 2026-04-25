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
input, select, button { width:100%; padding:10px; margin-top:10px; }
button { background:#2563eb; color:white; border:none; cursor:pointer; }
</style>
</head>
<body>
<div class='card'>
<h2>Configuração WiFi</h2>
<form action='/save' method='POST'>

<label>SSID (manual)</label>
<input type='text' name='ssid_manual' placeholder='Digite o SSID'>

<label>Ou selecione uma rede</label>
<select name='ssid_select'>
<option value=''>-- Escolher rede --</option>
)rawliteral";

const char PAGE_PORTAL_2[] PROGMEM = R"rawliteral(
</select>

<label>Senha WiFi</label>
<input type='password' name='wifi_pass'>

<label>Senha Web (PasswordWS)</label>
<input type='text' name='ws_pass' placeholder='Opcional' value=')rawliteral";

const char PAGE_PORTAL_2B[] PROGMEM = R"rawliteral('>

<label>Grupo MQTT</label>
<input type='text' name='grupo' value=')rawliteral";

const char PAGE_PORTAL_2C[] PROGMEM = R"rawliteral('>

<label>Hostname</label>
<input type='text' name='hostname' value=")rawliteral";

const char PAGE_PORTAL_3[] PROGMEM = R"rawliteral(">

<label>Modo de IP</label>
<select name='ip_mode' id='ip_mode' onchange="toggleIP(this.value)">
  <option value='dhcp'>DHCP</option>
  <option value='static'>IP Fixo</option>
</select>

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
function toggleIP(v){
  document.getElementById('ip_fields').style.display = v==='static'?'block':'none';
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

<link rel="stylesheet" href="/style.css" />

</head>
<body>

<nav class="navbar">
  <button class="navbar-burger" onclick="drawerOpen()" aria-label="Menu">
    &#9776;
  </button>
  <span class="navbar-brand" id="name">IRHub-8266</span>
  <div class="navbar-links">
    <a href="/">Home</a>
    <a href="/system">System</a>
  </div>
</nav>

<div class="drawer-overlay" id="drawerOverlay" onclick="drawerClose()"></div>
<div class="drawer" id="drawer">
  <div class="drawer-title">IRHub-8266</div>
  <a href="/">&#x1F3E0; Home</a>
  <a href="/system">&#x1F916; System</a>
</div>

<div class="page-content">


  <h2>📁 LittleFS File Manager</h2>

<div class="card">
  <h3>📤 Upload de arquivo</h3>

  <div class="upload-row">
    <input type="file" id="file">
    <button class="btn-send" onclick="upload()">📤 Enviar</button>
  </div>

  <progress id="prog" value="0" max="100"></progress>
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

<script>
function upload(){
  const file=document.getElementById('file').files[0];
  if(!file){alert('Selecione um arquivo');return;}

  const xhr=new XMLHttpRequest();

  xhr.upload.onprogress=function(e){
    if(e.lengthComputable){
      document.getElementById('prog').value=(e.loaded/e.total)*100;
    }
  };

  xhr.onload=function(){
    alert('Upload concluído');
    window.location.reload();
  };

  const formData=new FormData();
  formData.append('upload',file);

  const pass = prompt('🔐 Informe a senha HTTP para continuar:');
  if (!pass) return;
  xhr.withCredentials=true;
  xhr.open('POST','/upload',true);
  xhr.setRequestHeader('Authorization', 'Basic ' + btoa('admin:' + pass));
  xhr.send(formData);
}
</script>

</body>
</html>
)rawliteral";