# 🎥 Transmissor de TALLY Sem Fio Multiplataforma (Active LOW)

Sistema de **TALLY light sem fio** de alta performance, projetado para produções audiovisuais e transmissões ao vivo de baixo custo. O projeto foi desenvolvido de forma independente com o auxílio de **Inteligência Artificial (IA)** para criar uma alternativa acessível a equipamentos de broadcast caríssimos.

## 🚀 Diferenciais do Projeto
* **Multiplataforma Nativo:** Funciona com os principais softwares e mesas do mercado atual:
  * **OBS Studio** (via WebSocket - Porta default 4455)
  * **vMix** (via TCP Tally)
  * **ATEM Blackmagic** (via UDP)
  * **Mesas Analógicas / Switchers Antigos** (Interface física via conector DB25 / i2C)
* **Lógica Ativa em LOW (Active LOW):** Garante maior estabilidade de sinal e compatibilidade com acionamentos eletrônicos industriais e relés.
* **Comunicação Sem Fio:** Envio estável dos estados de **PREVIEW** (Luz Verde) e **NO AR** (Luz Vermelha) para os cubos receptores das câmeras.

## 🛠️ Hardware Utilizado
* **Microcontrolador Central:** ESP32 / NodeMCU (com conectividade WiFi integrada).
* **Módulos Receptores:** M5StickC ou placas customizadas com displays/LEDs RGB.
* **Interface Física:** Conector DB25 para integração com chaveadores analógicos.

## 📁 Como Utilizar
1. Faça o clone ou baixe o código fonte contido neste repositório.
2. Abra o arquivo na IDE do Arduino.
3. Configure as credenciais da sua rede WiFi local no código.
4. Faça o upload para a sua placa de desenvolvimento.
5. Acesse a interface de configuração pelo navegador digitando o IP da placa para mapear os canais (Câmera 1 a 12).

## 🤝 Contribuições e Legado
Este projeto é **Open Source** (Código Aberto). Sinta-se livre para abrir *Issues*, propor melhorias no código (*Pull Requests*) ou adaptar para novos hardwares. O objetivo é fortalecer a comunidade *Maker* e o mercado de streaming independente no Brasil!

---
*Desenvolvido com orgulho por um desenvolvedor sênior de 67 anos e entusiasta de IA.* 🛠️
