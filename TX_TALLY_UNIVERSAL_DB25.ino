#include <ETH.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h> 
#include <Wire.h>         // Barramento I2C para expansores PCF8575
#include <ArduinoWebsockets.h>
#include <ATEMmax.h>
#include <ATEMstd.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "interface.h"

using namespace websockets;

void tratarPaginaRaiz();
void tratarSalvarConfig();

Preferences memoria;
WebServer servidor(80);
DNSServer dnsServer;
WebsocketsClient clienteWebsocket; // OBS WebSocket
WiFiClient clienteVmix;            // vMix TCP Client
ATEMstd atemSwitcher;              // ATEM UDP Switcher

IPAddress obsIP;
int obsPorta = 4455;               
int softwareTipo = 0;              // 0 = OBS, 1 = vMix, 2 = ATEM

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
typedef struct struct_message { 
    uint16_t pgm; // 16 Bits
    uint16_t pvw; // 16 Bits
} struct_message;

struct_message dadosTally;
struct_message ultimosDadosTally = {0xFFFF, 0xFFFF};

unsigned long ultimoEnvioHeartbeat = 0;
const unsigned long INTERVALO_HEARTBEAT = 500;
unsigned long ultimaTentativaConexao = 0;
unsigned long tempoReiniciar = 0;

String nomesCenas[12] = {"Canal 1", "Canal 2", "Canal 3", "Canal 4", "Canal 5", "Canal 6", "Canal 7", "Canal 8", "Canal 9", "Canal 10", "Canal 11", "Canal 12"};

void carregarConfiguracoes() {
  memoria.begin("config", true);
  String ipSalvo = memoria.getString("obs_ip", "192.168.1.50");
  obsIP.fromString(ipSalvo);
  
  softwareTipo = memoria.getInt("soft_tipo", 0);
  int portaPadrao = (softwareTipo == 0) ? 4455 : ((softwareTipo == 1) ? 8099 : 0);
  obsPorta = memoria.getInt("obs_porta", portaPadrao);

  String cenasSalvas = memoria.getString("cenas_lista", "Canal 1,Canal 2,Canal 3,Canal 4,Canal 5,Canal 6,Canal 7,Canal 8,Canal 9,Canal 10,Canal 11,Canal 12");
  memoria.end();

  int indexAtual = 0;
  for (int i = 0; i < 12; i++) {
    int proximaVirgula = cenasSalvas.indexOf(',', indexAtual);
    if (proximaVirgula == -1) {
      nomesCenas[i] = cenasSalvas.substring(indexAtual);
      break;
    }
    nomesCenas[i] = cenasSalvas.substring(indexAtual, proximaVirgula);
    indexAtual = proximaVirgula + 1;
  }
}

void tratarPaginaRaiz() { 
  String htmlModificado = String(html_pagina);
  htmlModificado.replace("#ip_atual#", obsIP.toString());
  htmlModificado.replace("#porta_atual#", String(obsPorta));
  htmlModificado.replace("#tipo_atual#", String(softwareTipo));
  
  String stringCenas = "";
  for(int i=0; i<12; i++) {
    stringCenas += nomesCenas[i] + (i < 11 ? "," : "");
  }
  htmlModificado.replace("#cenas_atuais#", stringCenas);
  servidor.send(200, "text/html", htmlModificado); 
}

void tratarSalvarConfig() {
  if (servidor.hasArg("obs_ip")) {
    memoria.begin("config", false);
    memoria.putString("obs_ip", servidor.arg("obs_ip"));
    
    if(servidor.hasArg("software_tipo")) {
      memoria.putInt("soft_tipo", servidor.arg("software_tipo").toInt());
    }
    if(servidor.hasArg("obs_porta")) {
      memoria.putInt("obs_porta", servidor.arg("obs_porta").toInt());
    }
    
    String stringParaSalvar = "";
    for(int i=0; i<12; i++) {
      String chaveArg = "cena" + String(i);
      if(servidor.hasArg(chaveArg)) {
        stringParaSalvar += servidor.arg(chaveArg) + (i < 11 ? "," : "");
      }
    }
    memoria.putString("cenas_lista", stringParaSalvar);
    memoria.end();
    
    servidor.send(200, "text/plain", "OK");
    tempoReiniciar = millis() + 3000; 
  }
}

// =================== DRIVERS DOS SWITCHERS ===================

void aoReceberMensagemOBS(WebsocketsMessage msg) {
  String json = msg.data();
  if (json.indexOf("\"op\":5") == -1) return; 

  if (json.indexOf("\"eventType\":\"CurrentProgramSceneChanged\"") != -1) {
    uint16_t pgmTmp = 0;
    for (int i = 0; i < 12; i++) {
      String buscaCena = "\"sceneName\":\"" + nomesCenas[i] + "\"";
      if (json.indexOf(buscaCena) != -1) { bitSet(pgmTmp, i); break; }
    }
    dadosTally.pgm = pgmTmp;
  }
  else if (json.indexOf("\"eventType\":\"CurrentPreviewSceneChanged\"") != -1) {
    uint16_t pvwTmp = 0;
    for (int i = 0; i < 12; i++) {
      String buscaCena = "\"sceneName\":\"" + nomesCenas[i] + "\"";
      if (json.indexOf(buscaCena) != -1) { bitSet(pvwTmp, i); break; }
    }
    dadosTally.pvw = pvwTmp;
  }
}

void conectarAoOBS() {
  if (!clienteWebsocket.available() && (millis() - ultimaTentativaConexao > 5000)) {
    ultimaTentativaConexao = millis();
    String url = "ws://" + obsIP.toString() + ":" + String(obsPorta);
    if (clienteWebsocket.connect(url)) {
      clienteWebsocket.send("{\"op\":1,\"d\":{\"rpcVersion\":1,\"eventSubscriptions\":4}}");
    }
  }
}

void gerenciarConexaoVmix() {
  if (!clienteVmix.connected()) {
    if (millis() - ultimaTentativaConexao > 5000) {
      ultimaTentativaConexao = millis();
      if (clienteVmix.connect(obsIP, obsPorta)) {
        delay(50);
        clienteVmix.print("SUBSCRIBE TALLY\r\n");
        clienteVmix.flush();
      }
    }
    return;
  }

  while (clienteVmix.available()) {
    String linha = clienteVmix.readStringUntil('\n');
    linha.replace("\r", ""); linha.trim();

    if (linha.startsWith("VERSION OK")) {
      delay(50);
      clienteVmix.print("SUBSCRIBE TALLY\r\n");
      clienteVmix.flush();
    }
    else if (linha.startsWith("TALLY OK")) {
      String dadosEstados = linha.substring(9);
      uint16_t pgmTmp = 0; uint16_t pvwTmp = 0;

      for (int i = 0; i < 12; i++) {
        if (i < dadosEstados.length()) {
          char estado = dadosEstados.charAt(i);
          if (estado == '1') bitSet(pgmTmp, i);
          else if (estado == '2') bitSet(pvwTmp, i);
        }
      }
      dadosTally.pgm = pgmTmp;
      dadosTally.pvw = pvwTmp;
    }
  }
}

void gerenciarATEMeHardware() {
  atemSwitcher.runLoop();

  uint16_t pgmTemporario = 0;
  uint16_t pvwTemporario = 0;

  if (atemSwitcher.isConnected()) {
    // Lê diretamente da mesa ATEM
    for (int i = 0; i < 12; i++) {
      if (atemSwitcher.getProgramTally(i + 1)) bitSet(pgmTemporario, i);
      if (atemSwitcher.getPreviewTally(i + 1)) bitSet(pvwTemporario, i);
    }
  } 
  else {
  // Fallback para o único PCF8575 (modo DB25 / I2C)
  Wire.requestFrom(0x20, 2);
  if (Wire.available() == 2) {
    uint8_t byte_PGM = Wire.read(); // P0–P7
    uint8_t byte_PVW = Wire.read(); // P8–P15

    for (int i = 0; i < 8; i++) {
      // Cada câmera usa dois pinos: PGM = P0x, PVW = P1x
      bool estadoPGM = bitRead(byte_PGM, i) == LOW;
      bool estadoPVW = bitRead(byte_PVW, i) == LOW;

      if (estadoPGM) bitSet(pgmTemporario, i);
      if (estadoPVW) bitSet(pvwTemporario, i);
    }

    Serial.print("PGM bits: ");
    Serial.println(byte_PGM, BIN);
    Serial.print("PVW bits: ");
    Serial.println(byte_PVW, BIN);
  } else {
    pgmTemporario = 0;
    pvwTemporario = 0;
  }
}


  dadosTally.pgm = pgmTemporario;
  dadosTally.pvw = pvwTemporario;
}



// =============================================================

void setup() {
  Serial.begin(115200);
  
  delay(500);

  // Inicializa barramento I2C do WT32-ETH01
  Wire.begin(14, 15);
  Wire.beginTransmission(0x20); 
  Wire.write(0xFF);
  Wire.write(0xFF);
  Wire.endTransmission();

  carregarConfiguracoes();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("Tally_TX_Config", "tally123", 1, 0); 
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // --- OTIMIZAÇÃO: Modo Portal Captivo (*) ---
  // Redireciona qualquer chamada no Wi-Fi para abrir a página automaticamente
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
  Serial.println("Servidor DNS Captivo iniciado em 192.168.4.1");

  if (esp_now_init() == ESP_OK) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_AP; 
    esp_now_add_peer(&peerInfo);
    Serial.println("Transmissor Universal Pronto!");
  }

  servidor.on("/", tratarPaginaRaiz);
  servidor.on("/salvar", tratarSalvarConfig);

  // Entrega o Manifesto de instalação do Aplicativo
  servidor.on("/manifest.json", []() {
    String manifest = R"rawliteral({
      "short_name": "TallyConfig",
      "name": "Configurador Tally Universal",
      "icons": [
        { "src": "/icone.png", "sizes": "512x512", "type": "image/png" }
      ],
      "start_url": "/",
      "background_color": "#1e1e24",
      "theme-color": "#1e1e24",
      "display": "standalone",
      "orientation": "portrait"
    })rawliteral";
    servidor.send(200, "application/json", manifest);
  });

  // Entrega o Service Worker mínimo para validação de segurança do Android/iOS
  servidor.on("/sw.js", []() {
    String sw = "self.addEventListener('fetch', function(e) { });";
    servidor.send(200, "application/javascript", sw);
  });

  // --- CORREÇÃO AQUI: Link do xmlns corrigido para validar o ícone no celular ---
  servidor.on("/icone.png", []() {
    String svgIcone = R"rawliteral(<svg xmlns="http://w3.org" width="512" height="512" viewBox="0 0 512 512">
      <rect width="512" height="512" rx="100" fill="#2a2a35"/>
      <circle cx="256" cy="256" r="180" fill="none" stroke="#00adb5" stroke-width="20"/>
      <circle cx="256" cy="256" r="40" fill="#ff5722"/>
      <path d="M190 350 L235 256 L277 256 L322 350" fill="none" stroke="#fff" stroke-width="25" stroke-linecap="round"/>
      <text x="256" y="160" font-family="Arial" font-size="80" font-weight="bold" fill="#00adb5" text-anchor="middle">TALLY</text>
    </svg>)rawliteral";
    servidor.send(200, "image/svg+xml", svgIcone); 
  });

  servidor.onNotFound(tratarPaginaRaiz);
  servidor.begin();

  // Mantém o mDNS ativo para você conseguir acessar digitando http://tallyconfig.local 
  // caso o seu computador ou iPhone estejam conectados no mesmo cabo de rede/roteador geral!
  if (MDNS.begin("tallyconfig")) {
    Serial.println("mDNS iniciado! Acesse por http://tallyconfig.local no cabo de rede");
  }
  
  ETH.begin(); 
  
  clienteWebsocket.onMessage(aoReceberMensagemOBS);
  
  atemSwitcher.begin(obsIP);
  atemSwitcher.connect();
}


void loop() {

  dnsServer.processNextRequest();
  
  servidor.handleClient();
  
  // Executa os modos de REDE apenas se o cabo estiver conectado
  if (ETH.linkUp()) {
    if (softwareTipo == 0) {
      conectarAoOBS();
      if (clienteWebsocket.available()) clienteWebsocket.poll();
      if (clienteVmix.connected()) clienteVmix.stop();
    } 
    else if (softwareTipo == 1) {
      gerenciarConexaoVmix();
      if (clienteWebsocket.available()) clienteWebsocket.close();
    }
    else if (softwareTipo == 2) {
      // Modo ATEM via Rede Pura
      atemSwitcher.runLoop();
      uint16_t pgmTmp = 0; uint16_t pvwTmp = 0;
      if (atemSwitcher.isConnected()) {
        for (int i = 0; i < 12; i++) {
          if (atemSwitcher.getProgramTally(i + 1)) bitSet(pgmTmp, i);
          if (atemSwitcher.getPreviewTally(i + 1)) bitSet(pvwTmp, i);
        }
      }
      dadosTally.pgm = pgmTmp;
      dadosTally.pvw = pvwTmp;
      
      if (clienteWebsocket.available()) clienteWebsocket.close();
      if (clienteVmix.connected()) clienteVmix.stop();
    }
  } else {
    if (clienteVmix.connected()) clienteVmix.stop();
  }

  // --- MODO 3: MESA ANALÓGICA / DB25 (Roda direto, independente de cabo de rede) ---
  if (softwareTipo == 3) {
    //lerMesaAnalogicaDB25();
     gerenciarATEMeHardware();
    // Derruba conexões de rede se estiverem abertas
    if (clienteWebsocket.available()) clienteWebsocket.close();
    if (clienteVmix.connected()) clienteVmix.stop();
  }

  // Despacha o sinal via rádio ESP-NOW
  if ((dadosTally.pgm != ultimosDadosTally.pgm) || 
      (dadosTally.pvw != ultimosDadosTally.pvw) || 
      (millis() - ultimoEnvioHeartbeat >= INTERVALO_HEARTBEAT)) {
    
    esp_now_send(broadcastAddress, (uint8_t *) &dadosTally, sizeof(dadosTally));
    ultimosDadosTally = dadosTally;
    ultimoEnvioHeartbeat = millis();
  }

  if (tempoReiniciar > 0 && millis() > tempoReiniciar) {
    ESP.restart();
  }
  
  delay(5);
}
