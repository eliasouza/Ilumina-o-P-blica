/*
* Adaptado de:
* Copyright (c) 2015, Majenko Technologies
* All rights reserved.
*
*/
 
/*
*
* Computer Science Section
* Faculdades Integradas de Caratinga
* Caratinga, MG, Brazil
* November 04, 2017
* author: Majenko Technologies
* adapter: Elias Goncalves
* email: falarcomelias@gmail.com
*
*/

//************************************************************************************************************************
// O que faz esse codigo?
//************************************************************************************************************************
// 1. Cria um ponto de acesso (AP);
// 2. Disponibiliza um web server;
//************************************************************************************************************************

//************************************************************************************************************************
// Incluir as bibliotecas:
//************************************************************************************************************************
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

//************************************************************************************************************************
// Configuracao das credenciais de acesso ao AP:
//************************************************************************************************************************
const char *ssid = "SIMPIP";
const char *password = "simpip123";

ESP8266WebServer server(80);

/* 
* Para uma mensagem de teste apos se conectar ao
* AP acesse: http://192.168.4.1 no navegador web.
*/
void handleRoot() {
  server.send(200, "text/html", "<h1>Conectado.</h1>");
}

//************************************************************************************************************************
// Configuracao do programa:
//************************************************************************************************************************
void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configurando ponto de acesso...");

  // Para usar o AP sem senha, nao passar o parametro password;
  WiFi.softAP(ssid, password);

  IPAddress ip = WiFi.softAPIP();
  Serial.print("IP do ponto de acesso: ");
  Serial.println(ip);
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Servidor HTTP inicializado.");
}

void loop() {
  server.handleClient();
}
