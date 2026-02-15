// ==========================
// Conectividade Wi-Fi
// ==========================

// #include <ESP8266WiFi.h>
// Controla a interface Wi-Fi do ESP8266.
// Permite conectar a redes (modo Station),
// criar Access Point, obter IP, RSSI e gerenciar reconexões.

// #include <ESP8266WebServer.h>
// Implementa um servidor HTTP embutido.
// Permite criar rotas ("/", "/status"), enviar HTML/JSON
// e receber parâmetros via GET/POST.


// ==========================
// Comunicação MQTT
// ==========================

// #include <PubSubClient.h>
// Cliente MQTT leve.
// Permite publicar mensagens e assinar tópicos em um broker.
// Essencial para integração com Home Assistant, Node-RED etc.


// ==========================
// OTA (Atualização via rede)
// ==========================

// #include <ESP8266mDNS.h>
// Implementa mDNS (Multicast DNS).
// Permite acessar o ESP por nome (ex: esp8266.local)
// e é usado para descoberta automática no OTA.

// #include <WiFiUdp.h>
// Implementa comunicação UDP.
// Necessário para funcionamento interno do ArduinoOTA.

// #include <ArduinoOTA.h>
// Permite atualizar o firmware via Wi-Fi (Over-The-Air).
// Evita a necessidade de cabo USB após instalação inicial.


// ==========================
// Controle Infravermelho (IR)
// ==========================

// #include <IRremoteESP8266.h>
// Biblioteca base para controle IR no ESP8266.
// Define protocolos e estruturas utilizadas pelas demais libs IR.

// #include <IRrecv.h>
// Responsável por receber sinais IR.
// Captura e decodifica pulsos vindos do receptor infravermelho.

// #include <IRsend.h>
// Responsável por enviar sinais IR.
// Gera pulsos precisos no LED IR conforme protocolo escolhido.

// #include <IRac.h>
// API de alto nível para controle de ar-condicionado.
// Permite definir modo, temperatura, ventilação etc.
// sem precisar montar manualmente o protocolo IR.

// #include <IRtext.h>
// Converte códigos IR em texto legível.
// Útil para logs e debug via Serial/MQTT/Telnet.

// #include <IRutils.h>
// Funções auxiliares para manipulação e formatação de dados IR.
// Facilita debug e tratamento de protocolos.


// ==========================
// Comunicação I2C e Sensor
// ==========================

// #include <Wire.h>
// Implementa o barramento I2C (SDA/SCL).
// Permite comunicação com sensores e dispositivos externos.

// #include "Adafruit_AHTX0.h"
// Driver para sensores AHT10/AHT20.
// Fornece leitura de temperatura e umidade com compensação interna.

/************ AHT10 ************/
/*==================================
  Configuração e descrições dos pinos

  Pin Número 	  Nome 	    Descrição
  1 	          VDD 	    Fonte de alimentação (2,2 V a 5,5 V)
  2 	          SDA 	    I2C Data line
  3 	          GND 	    Ground
  4 	          SCL 	    I2C Clock line
  ==================================*/

/************ FEEDBACK ************/
/*
    --------------------------------------------------------------
    Realizar impressão simples de dados formatados
    Especificadores de conversão suportados:
         d, i     signed int
         u        unsigned int
         ld, li   signed long
         lu       unsigned long
         f        double
         c        char
         s        string
         %        '%'
    Usage: %[conversion specifier]
    Note: This function does not support these format specifiers:
         [flag][min width][precision][length modifier]
    --------------------------------------------------------------
  */
