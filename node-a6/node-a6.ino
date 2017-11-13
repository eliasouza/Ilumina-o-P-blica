/*
*
* Computer Science Section
* Faculdades Integradas de Caratinga
* Caratinga, MG, Brazil
* July 19, 2017
* author: Elias Goncalves
* email: falarcomelias@gmail.com
*
*/

//************************************************************************************************************************
// O que faz esse codigo?
//************************************************************************************************************************
// 1. Recebe pacote de mensagem de todos os nos da rede;
// 2. Verifica se algum no esta com problema (lampada acesa/apagada em horario indevido);
// 3. Envia SMS para a central com o A6.
//************************************************************************************************************************

//************************************************************************************************************************
// Incluir as bibliotecas:
//************************************************************************************************************************
#include "EmonLib.h"
#include <SPI.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <virtuabotixRTC.h>

//************************************************************************************************************************
// Configuracao do acesso a rede Mesh:
//************************************************************************************************************************
#define MESH_SSID "SIMPIP" // Nome da sua rede Mesh;
#define MESH_PASSWORD "simpip123" // Senha da sua rede Mesh;
#define MESH_PORT 5555 // Porta de comunicacao da rede Mesh.

//************************************************************************************************************************
// Declaracao de variaveis e Constantes:
//************************************************************************************************************************
// MESH:
painlessMesh mesh; // Instancia o objeto mesh;
SimpleList<uint32_t> nodes; // Cria uma lista para os nos da rede mesh (cada novo no conectado e add nela);

// A6:
char telefone[] = "999058910"; // 988338506 Coloque aqui o numero de telefone que recebera a mensagem SMS.

// RTC DS1302:
virtuabotixRTC RTC(D5, D6, D7); // Pinos ligados ao modulo. Parametros: RTC(clock, data, rst)
int ultimoEnvio = -1;

// Dados tratados:
double corrente = 0.00;
int luminosidade = 0;
String hora = "";
String no = "";

String msg = String("0.23!254!19:54!NodeMCU#1");

//************************************************************************************************************************
// Configuracao de debug:
//************************************************************************************************************************
#define DEV false // Ativa/desativa o modo desenvolvedor para mostrar os debugs na serial;

//************************************************************************************************************************
// Configuracao do programa:
//************************************************************************************************************************
void setup() {
    Serial.begin(9600); // Velocidade de comunicacao;

    //Mensagens de Debug (tem que ser configurado antes da inicializacao da mesh para ver mensagens de erro de inicializacao, se ocorrerem):
    mesh.setDebugMsgTypes(ERROR | DEBUG);  // De todos tipos opcionais, foram escolhidas somete essas duas.

    // Inicializacao da rede mesh:
    mesh.init(MESH_SSID, MESH_PASSWORD, MESH_PORT); // Configura a rede mesh com os dados da rede sem fio;
    mesh.onReceive(&receivedCallback); // Chamada da funcao ao receber uma mensagem;
    mesh.onNewConnection(&newConnectionCallback); // Chamada da funcao quando um novo no se conecta a rede mesh;
    mesh.onChangedConnections(&changedConnectionCallback); // Chamada da funcao ao mudar de conexao;

    // Configuraçes iniciais de data e hora - Apos configurar, comentar a linha abaixo;
    // RTC.setDS1302Time(00, 11, 15, 4, 1, 11, 2017); // Formato: (segundos, minutos, hora, dia da semana, dia do mes, mes, ano);
}

//************************************************************************************************************************
// Execucao do programa:
//************************************************************************************************************************
void loop() {

    // Atualizar informacoes da rede mesh:
    mesh.update();
    
    // Horario:
    //readTime();
    
    char buf[sizeof(msg)];
    
    // Percorrer a String e dividir em partes:
    msg.toCharArray(buf, sizeof(buf));
    char *p = buf;
    char *str;
    
    while ((str = strtok_r(p, "!", &p)) != NULL){
      
      // Converter e atribuir a cada variavel conforme o tipo.
      Serial.println(str);
    }
 }

//************************************************************************************************************************
// Funcoes pessoais:
//************************************************************************************************************************
void readTime(){ // Lendo horario
  RTC.updateTime(); // Le as informacoes do circuito;

  if(DEV){
    // Imprime as informacoes no serial monitor:
    Serial.print("Data: ");
    Serial.print(RTC.dayofmonth);
    Serial.print("/");
    Serial.print(RTC.month);
    Serial.print("/");
    Serial.print(RTC.year);
  }
}

void sendSMS(String msg){ // Envia SMS para o numero cadastrado.
    Serial.println("AT+CMGF=1");
    delay(2000);
    Serial.print("AT+CMGS=\"");
    Serial.print(telefone);
    Serial.write(0x22);
    Serial.write(0x0D);
    Serial.write(0x0A);
    delay(2000);
    Serial.print(msg);
    delay(500);
    Serial.println (char(26));
    
    // Atualiza a hora do envio
    ultimoEnvio = RTC.hours;
}

//************************************************************************************************************************
// Funcoes Callback (Chamadas sempre que ocorre um evento):
//************************************************************************************************************************
void receivedCallback(uint32_t from, String &msg) {
    Serial.print("Mensagem recebida: ");
    Serial.println(msg);
 
 /*   
    // Percorrer a String e dividir em partes:
    const char *msgRecebida = msg.toCharArray(10, 25);
    char *token, *str, *tofree;
    tofree = str = strdup(msgRecebida);
    
    while ( (token = strsep(&str, "!")) ){
      
      // Converte cada um conforme seu tipo
      Serial.println(token);
    }
    free(tofree);
  */
}

void changedConnectionCallback() { // Mudando de conexo
    Serial.printf("Conexao alterada (%s)\n", mesh.subConnectionJson().c_str());
    nodes = mesh.getNodeList();
    Serial.printf("Quantidade de no(s) conectado(s): %d\n", nodes.size());
    Serial.printf("Lista de conexoes:\n");
    SimpleList<uint32_t>::iterator node = nodes.begin();
    
    while (node != nodes.end()) {           
      Serial.printf(" %s", *node);
      node++;
    }
    
    Serial.println();
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("Nova conexao com no cujo id e: %u\n", nodeId);
}
