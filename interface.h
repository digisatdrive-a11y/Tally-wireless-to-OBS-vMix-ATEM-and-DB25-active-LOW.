#ifndef interface_h
#define interface_h

#include <Arduino.h>

const char html_pagina[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Configuração Tally Universal</title>
<!-- Tags para PWA / Web App Móvel -->
<link rel="manifest" href="/manifest.json">
<meta name="theme-color" content="#1e1e24">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="Tally Config">
<link rel="apple-touch-icon" href="/icone.png">

<style>
body{font-family:Arial,sans-serif;background:#1e1e24;color:#fff;text-align:center;padding:20px;margin:0;}
.container{max-width:500px;margin:20px auto;background:#2a2a35;padding:25px;border-radius:12px;box-shadow:0 5px 15px rgba(0,0,0,0.3);}
h2{color:#00adb5;margin-bottom:20px;}
.grupo-form{text-align:left;margin-bottom:15px;}
label{font-size:14px;color:#bbb;display:block;margin-bottom:5px;font-weight:bold;}
input[type="text"], select{width:100%;padding:10px;border-radius:6px;border:1px solid #444;background:#1e1e24;color:#fff;font-size:15px;box-sizing:border-box;}
.grid-cenas{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:15px;}
button{background:#00adb5;color:#fff;border:none;padding:14px;font-size:16px;border-radius:6px;width:100%;cursor:pointer;font-weight:bold;margin-top:20px;transition:0.2s;}
button:hover{background:#008c94;}
button:disabled{background:#555;cursor:not-allowed;}
/* Estilo do botão de instalação do Aplicativo */
#btnInstalar{background:#ff5722;display:none;margin-bottom:20px;animation:pulsar 2s infinite;}
@keyframes pulsar { 0% {transform: scale(1);} 50% {transform: scale(1.03);} 100% {transform: scale(1);} }
</style></head>
<body><div class="container">
<button id="btnInstalar">📲 Instalar Aplicativo Tally</button>
<h2>Configurações do Tally</h2>
<form id="meuFormulario">

<div class="grupo-form">
  <label>Software / Mesa de Vídeo:</label>
  <select name="software_tipo" id="software_tipo" onchange="ajustarPortaPadrao()">
    <option value="0">OBS Studio (WebSocket)</option>
    <option value="1">vMix (TCP Tally)</option>
    <option value="2">ATEM Blackmagic (UDP)</option>
    <option value="3">Mesa Analógica / DB25 (I2C)</option>
  </select>
</div>

<div class="grupo-form" id="bloco_ip"><label>IP do Equipamento (OBS/vMix/ATEM):</label><input type="text" name="obs_ip" id="obs_ip" required></div>
<div class="grupo-form" id="bloco_porta"><label>Porta de Conexão:</label><input type="text" name="obs_porta" id="obs_porta" required></div>

<h3 style="color:#00adb5;margin-top:25px;font-size:16px;border-top:1px solid #444;padding-top:15px;">Mapeamento de Canais (1 a 12)</h3>
<div class="grid-cenas" id="containerCenas"></div>
<button type="submit" id="btnSalvar">Salvar Configurações</button></form></div>
<script>
const ipAtual = "#ip_atual#";
const portaAtual = "#porta_atual#";
const tipoAtual = "#tipo_atual#";
const cenasAtuais = "#cenas_atuais#".split(",");

document.getElementById("obs_ip").value = ipAtual;
document.getElementById("obs_porta").value = portaAtual;
document.getElementById("software_tipo").value = tipoAtual;

const container = document.getElementById("containerCenas");
for(let i=0; i<12; i++){
  container.innerHTML += `<div class="grupo-form"><label>Canal ${i+1}:</label><input type="text" name="cena${i}" value="${cenasAtuais[i] || 'Canal '+(i+1)}" required></div>`;
}

function ajustarPortaPadrao() {
  const tipo = document.getElementById("software_tipo").value;
  const campoPorta = document.getElementById("obs_porta");
  const blocoPorta = document.getElementById("bloco_porta");
  const blocoIp = document.getElementById("bloco_ip");
  const campoIp = document.getElementById("obs_ip");
  
  campoIp.required = true;

  if(tipo === "0") {
    campoPorta.value = "4455";
    blocoPorta.style.display = "block";
    blocoIp.style.display = "block";
  } else if(tipo === "1") {
    campoPorta.value = "8099";
    blocoPorta.style.display = "block";
    blocoIp.style.display = "block";
  } else if(tipo === "2") {
    campoPorta.value = "0";
    blocoPorta.style.display = "none";
    blocoIp.style.display = "block";
  } else {
    campoPorta.value = "0";
    campoIp.value = "0.0.0.0";
    campoIp.required = false;
    blocoPorta.style.display = "none";
    blocoIp.style.display = "none";
  }
}
ajustarPortaPadrao();

document.getElementById("meuFormulario").addEventListener("submit", function(event) {
  event.preventDefault();
  const botao = document.getElementById("btnSalvar");
  botao.disabled = true;
  botao.innerText = "Salvando...";

  const dadosFormulario = new URLSearchParams(new FormData(this));

  fetch("/salvar", {
    method: "POST",
    body: dadosFormulario,
    headers: { "Content-Type": "application/x-www-form-urlencoded" }
  })
  .then(response => { 
    botao.innerText = "Salvo, reiniciando...";
    setTimeout(() => { alert("Configuração Salva com sucesso!\nO Transmissor está reiniciando."); }, 100);
  })
  .catch(error => { 
    botao.innerText = "Salvo, reiniciando...";
    setTimeout(() => { alert("Configuração Salva com sucesso!\nO Transmissor está reiniciando."); }, 100);
  });
});

// ---- LOGICA DE INSTALAÇÃO DO APLICATIVO ----
let deferredPrompt;
const btnInstalar = document.getElementById('btnInstalar');

window.addEventListener('beforeinstallprompt', (e) => {
  e.preventDefault();
  deferredPrompt = e;
  btnInstalar.style.display = 'block'; // Mostra o botão se puder instalar
});

btnInstalar.addEventListener('click', async () => {
  if (deferredPrompt) {
    deferredPrompt.prompt();
    const { outcome } = await deferredPrompt.userChoice;
    if (outcome === 'accepted') {
      btnInstalar.style.display = 'none';
    }
    deferredPrompt = null;
  }
});

window.addEventListener('appinstalled', () => {
  btnInstalar.style.display = 'none';
  deferredPrompt = null;
});

// Registra o Service Worker obrigatorio para rodar como App móvel
if ('serviceWorker' in navigator) {
  navigator.serviceWorker.register('/sw.js').catch(() => {});
}
</script></body></html>
)rawliteral";

#endif
