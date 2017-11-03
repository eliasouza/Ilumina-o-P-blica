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
// 1. Le sensor de corrente sct;
// 2. Le sensor de luminosidade ldr;
// 3. Le horario com RTC DS1302;
// 4. Envia mensagem broadcast;
// 5. Recebe mensagem broadcast callback;
// 6. Envia SMS com A6.
//************************************************************************************************************************

//************************************************************************************************************************
// Incluir as bibliotecas:
//************************************************************************************************************************
#include "EmonLib.h"
#include "painlessMesh.h"
#include <SPI.h>
//#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <virtuabotixRTC.h>

//************************************************************************************************************************
// Configuracao do acesso a rede Mesh:
//************************************************************************************************************************
#define MESH_SSID "SIMPIP" // Nome da sua rede Mesh;
#define MESH_PASSWORD "simpip" // Senha da sua rede Mesh;
#define MESH_PORT 5555 // Porta de comunicacao da rede Mesh.

//************************************************************************************************************************
// Declaracao de variaveis e Constantes:
//************************************************************************************************************************
// MESH:
painlessMesh mesh; // Instancia o objeto mesh;
SimpleList<String> expected;
SimpleList<String> received;
bool sendingDone = false;

// SCT E LDR:
EnergyMonitor emon1; // Monitor de energia para o sensor de corrente;
double numVoltasBobina = 2000; // Numero de voltas na bobina do leitor de corrente;
double resistorCarga = 22; // Carga do resistor;
#define SELETOR D1 // Pino responsavel por selecionar um dos dois sensores analogicos (0: LDR, 1: SCT);
#define PINO_LED D4 // Pino responsavel por ligar o LED;
#define PINO_MUX A0 // Pino responsavel por ler a entrada analogica selecionada pelo seletor;
int ldrValor = 0; // Valor lido do LDR;
double Irms = 0.00; // Valor lido do leitor corrente;
char telefone[] = "999058910"; // 988338506 Coloque aqui o numero de telefone que recebera a mensagem SMS.
#define CLARO 700 // Cosiderado claro ate 700 de luminosidade;
#define CORRENTE 0.25
int estabilizador = 0;

// RTC DS1302:
virtuabotixRTC RTC(D5, D6, D7); // Pinos ligados ao modulo. Parametros: RTC(clock, data, rst)
int ultimoEnvio = -1;

//************************************************************************************************************************
// Configuracao de debug:
//************************************************************************************************************************
#define DEV true // Ativa/desativa o modo desenvolvedor para mostrar os debugs na serial;

//************************************************************************************************************************
// Configuracao do programa:
//************************************************************************************************************************
void setup() {
    Serial.begin(9600); // Velocidade de comunicacao;
    pinMode(PINO_LED, OUTPUT); // Define o pino do led como saida
    pinMode(SELETOR, OUTPUT); // Define o seletor como saida

    //Mensagens de Debug (tem que ser configurado antes da inicializacao da mesh para ver mensagens de erro de inicializacao, se ocorrerem):
    mesh.setDebugMsgTypes(ERROR | DEBUG);  // De todos tipos opcionais, foram escolhidas somete essas duas.

    // Inicializacao da rede mesh:
    mesh.init(MESH_SSID, MESH_PASSWORD, MESH_PORT); // Configura a rede mesh com os dados da rede sem fio;
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onReceive(&receivedCallback); // Chamada da funcao ao receber uma mensagem;

    // Calibracao do pino
    emon1.current(PINO_MUX, (numVoltasBobina / resistorCarga));

    // Configuraçes iniciais de data e hora - Apos configurar, comentar a linha abaixo;
    // RTC.setDS1302Time(00, 11, 15, 4, 1, 11, 2017); // Formato: (segundos, minutos, hora, dia da semana, dia do mes, mes, ano);
}

//************************************************************************************************************************
// Execucao do programa:
//************************************************************************************************************************
void loop() {

    // Atualizar informacoes da rede mesh:
    mesh.update();

    // Corrente:
    readAmperage();

    // Luminosidade:
    readLuminosity();
    
    // Horario:
    readTime();
 }

//************************************************************************************************************************
// Funcoes pessoais:
//************************************************************************************************************************
void changeSensor(int state) {// Mudar a entrada do mux conforme o seletor
  digitalWrite(SELETOR, state);
}

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

  // Esta de noite - Lampadada apagada?
  if (RTC.hours >= 19 || RTC.hours < 7){// Esta entre 7h da noite e 6h da manha.

    // Verifica corrente e luminosidade
    if(Irms < CORRENTE){ // Pouca corrente nao importa se esta claro.

        // Envia a uma mensagem persistindo envia mais na hora seguinte.
        // MSG: Alerta de ponto sem iluminacao em horario noturno.
        if(RTC.hours > ultimoEnvio){
          sendSMS("Ponto sem iluminacao em horario noturno. Endereco...");
        }
        
        // Funcionamento normal: Central ja foi notificada.
        else{
          if(DEV){Serial.print("\nLampada apagada durante a noite! Central ja notificada.\n");}
        }
    }
    else{
      if(DEV){Serial.print("\nFuncionamento normal: noite com lampada acesa (ainda que a noite esteja clara).\n");}
    }

    if(DEV){
      Serial.print("\nNOITE.");
      Serial.print(" Hora: ");
      Serial.print(RTC.hours);
      Serial.print(":");
      Serial.print(RTC.minutes);
      Serial.print(":");
      Serial.println(RTC.seconds);
      if(ultimoEnvio != -1){
        Serial.print("Ultimo envio de SMS: ");
        Serial.print(ultimoEnvio);
        Serial.print("h\n");
      }
    }
  }

  // Esta em horario considerado dia
  else{

    // Verifica corrente e luminosidade
    if(Irms >= CORRENTE && ldrValor <= CLARO){ // Corrente acima do permitido para o horario e luminosidade indicando que esta claro.

      // Envia a uma mensagem persistindo envia mais na hora seguinte.
      // MSG: Ponto de iluminacao aceso durante o dia.
      if(RTC.hours > ultimoEnvio){
        sendSMS("Ponto de iluminacao aceso durante o dia. Endereco...");
      }
      
      // Funcionamento normal: Central ja foi notificada.
      else{
        if(DEV){Serial.print("\nLampada acesa durante o dia! Central ja notificada.\n");}
      }
    }
    
    // Funcionamentos normais:
    else if(Irms < CORRENTE && ldrValor > CLARO){
      
      // Envia a uma mensagem persistindo envia mais na hora seguinte.
      // MSG: Ponto de iluminacao aceso durante o dia.
      if(RTC.hours > ultimoEnvio){
        sendSMS("Ponto de iluminacao apagado durante dia escuro. Endereco...");
      }
      
      // Funcionamento normal: Central ja foi notificada.
      else{
        if(DEV){Serial.print("\nLampada apagada durante dia escuro! Central ja notificada.\n");}
      }
    }
    else{
      if(DEV){Serial.print("\nFuncionamento normal: dia claro com lampada apagada.\n");}
    }
    
    if(DEV){
      // Imprime as informacoes no serial monitor:
      Serial.print("\nDIA.");
      Serial.print(" Hora: ");
      Serial.print(RTC.hours);
      Serial.print(":");
      Serial.print(RTC.minutes);
      Serial.print(":");
      Serial.println(RTC.seconds);
      if(ultimoEnvio != -1){
        Serial.print("Ultimo envio de SMS: ");
        Serial.print(ultimoEnvio);
        Serial.print("h\n");
      }
    }
  }
}

void readAmperage(){ // Lendo corrente
  changeSensor(HIGH); // Seleciona no mux o leitor de corrente;
  Irms = emon1.calcIrms(1480); //Calcula a corrente

  if(DEV){
    // Mostra o valor da corrente no serial monitor:
    Serial.print("Corrente: ");
    Serial.println(Irms);
  }

  delay(1);
}

void readLuminosity(){ // Lendo luminosidade
   changeSensor(LOW);// Seleciona o leitor de luminosidade;
   ldrValor = analogRead(PINO_MUX); // O valor lido será entre 0 e 1023

   if(ldrValor >= CLARO){
     digitalWrite(PINO_LED, HIGH);
     if(DEV){Serial.println("ESCURO.");}
   }
   else{
     digitalWrite(PINO_LED, LOW);
     if(DEV) {Serial.println("CLARO.");}
   }

   if(DEV){
     // Imprime o valor lido do LDR no monitor serial:
     Serial.print("\nLuminosidade: ");
     Serial.println(ldrValor);
   }

   delay(1);
}

void sendSMS(String msg){ // Envia SMS para o numero cadastrado.
  if(estabilizador > 5){
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
  else{
    estabilizador ++;
  }
}

//************************************************************************************************************************
// Funcoes Callback (Chamadas sempre que ocorre um evento):
//************************************************************************************************************************
void receivedCallback(uint32_t from, String &msg) {
    received.push_back(msg);
    Serial.printf("CB: %d, %d\n", expected.size(), received.size());
    //Serial.printf("Mensagem recebida do %u: %s\n", from.c_str(), msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) { // Envia mensagem para o no que conectou
    String no = "";
    switch (mesh.getNodeId()) {
      case 1508618939:
        no = "NodeMCU1";
        break;
      case 1509656251:
        no = "NodeMCU2";
        break;
      case 1507515130:
        no = "NodeMCU3";
        break;
      case 1507515133:
        no = "NodeMCU4";
        break;
      default:
        no = "Nenhum";
        break;
    }

    // Formato da mensagem enviada: corrente!luminosidade!hora!no
    // Ex.:  0.20!325!18!NodeMCU #1
    String corrente = String(Irms);
    String luminosidade = String(ldrValor);
    String hora = String(RTC.hours);
    String nodeMCU = String(no);
    String msg = String(corrente + "!" + luminosidade + "!" + hora + "!" + nodeMCU);

    if ( mesh.sendSingle(nodeId, msg) )
        expected.push_back(msg);

    sendingDone = true;
}
