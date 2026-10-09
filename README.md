Projeto feito com ajuda de IA usando um ESP32-ETH01. 
Através da porta LAN conectada a uma rede Ethernet encontra-se o gerador de TALLY OBS ou vMix ou ATEM. 
Também pode-se usar o DB25 para um TALLY convencional ativo LOW.
A configuração é feita via Access Point gerado pelo ESP32, acessando o WI-FI "Tally_TX_Config", senha "tally123", Pelo browser digitar - "tallyconfig.local" - 
Pela pagina Web, você escolhe a fonte de Tally OBS, vMix ATEM e configura o IP correspondente, ou o DB25 para tally ativo LOW. 
O esp_now transmite a mudança de status sempre que houver alguma alteração ou a cada 250ms para garantir que a conecção esta ativa no RX.
O receptor foi feito com um ESP32-C3 TFT Display, tipo chaveiro.  
