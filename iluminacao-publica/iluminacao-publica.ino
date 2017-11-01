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
// 3. Le horario;
// 4. Envia mensagem broadcast;
// 5. Recebe mensagem broadcast callback;
// 6. Envia SMS.
//************************************************************************************************************************

//************************************************************************************************************************
// Incluir as bibliotecas:
//************************************************************************************************************************
#include "EmonLib.h"
#include <SPI.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>

//************************************************************************************************************************
//Configuracao do acesso ao roteador (sua rede sem fio):
//************************************************************************************************************************
#define MESH_SSID "TCC" // Nome da sua rede Mesh;
#define MESH_PASSWORD "tcc123" // Senha da sua rede Mesh;
#define MESH_PORT 5555 // Porta de comunicacao da rede Mesh.

//************************************************************************************************************************
// Declaracao de variaveis e Constantes:
//************************************************************************************************************************
// MESH:
painlessMesh mesh; // Instancia o objeto mesh;

// SCT E LDR:
EnergyMonitor emon1; // Monitor de energia para o sensor de corrente;
double numVoltasBobina = 2000; // Numero de voltas na bobina do leitor de corrente;
double resistorCarga = 22; // Carga do resistor;
#define SELETOR D1 // Pino responsavel por selecionar um dos dois sensores analogicos (0: LDR, 1: SCT);
#define PINO_LED D4 // Pino responsavel por ligar o LED;
#define PINO_MUX A0 // Pino responsavel por ler a entrada analogica selecionada pelo seletor;
int ldrValor = 0; // Valor lido do LDR;
double Irms = 0; // Valor lido do leitor corrente;
char telefone[] = "xxxxxxxx"; // Coloque aqui o numero de telefone que recebera a mensagem SMS.

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
    mesh.onReceive(&receivedCallback); // Chamada da funcao ao receber uma mensagem;

    // Calibracao do pino
    emon1.current(PINO_MUX, (numVoltasBobina / resistorCarga));
}

//************************************************************************************************************************
// Execucao do programa:
//************************************************************************************************************************
void loop() {

    // Atualizar informacoes da rede mesh:
    mesh.update();

    // Ler corrente:
    readAmperage();

    // Ler luminosidade:
    readLuminosity();
    
    // Gastando energia de dia - envia sms
    if(Irms > 0.30 && ldrValor <= 500){sendSMS();}
}

//************************************************************************************************************************
// Funcoes pessoais:
//************************************************************************************************************************
void changeSensor(int state) {// Mudar a entrada do mux conforme o seletor
  digitalWrite(SELETOR, state);
}

void readAmperage(){ // Lendo corrente
  changeSensor(HIGH); // Seleciona no mux o leitor de corrente;
  Irms = emon1.calcIrms(1480); //Calcula a corrente

  // Mostra o valor da corrente no serial monitor:
  Serial.print("\nCorrente: ");
  Serial.println(Irms);
  delay(1);
}

void readLuminosity(){ // Lendo luminosidade
   changeSensor(LOW);// Seleciona o leitor de luminosidade;
   ldrValor = analogRead(PINO_MUX); // O valor lido será entre 0 e 1023

   if(ldrValor >= 500){digitalWrite(PINO_LED, HIGH);}
   else{digitalWrite(PINO_LED, LOW);}

   // Imprime o valor lido do LDR no monitor serial:
   Serial.print("\nLuminosidade: ");
   Serial.println(ldrValor);
   delay(1);
}

void sendSMS(){ // Envia SMS para o numero cadastrado.
  Serial.println("AT+CMGF=1");    
  delay(2000);
  Serial.print("AT+CMGS=\"");
  Serial.print(telefone); 
  Serial.write(0x22);
  Serial.write(0x0D);
  Serial.write(0x0A);
  delay(2000);
  Serial.print("Falha de energia detectada no bairro x, rua y.");
  delay(500);
  Serial.println (char(26));
}

void sendMessage(){ // Enviando mensagens
    String no = "";
    switch (mesh.getNodeId()) {
      case 1508618939:
        no = "NodeMCU #1";
        break;
      case 1509656251:
        no = "NodeMCU #2";
        break;
      case 1507515130:
        no = "NodeMCU #3";
        break;
      case 1507515133:
        no = "NodeMCU #4";
        break;
      default:
        no = "Nenhum";
        break;
    }
    String msg = "|corrente|horario|luminosidade| do " + no;
    //String msg = "|corrente|horario|luminosidade| do " + mesh.getNodeId();

    Serial.println(msg);

    // Envia para todos os nos da rede mesh.
    mesh.sendBroadcast( msg );
}

//************************************************************************************************************************
// Funcoes Callback (Chamadas sempre que ocorre um evento):
//************************************************************************************************************************
void receivedCallback(uint32_t from, String &msg) { // Recebendo mensagens
    String no = "";
    switch (from) {
      case 1508618939:
        no = "NodeMCU #1";
        break;
      case 1509656251:
        no = "NodeMCU #2";
        break;
      case 1507515130:
        no = "NodeMCU #3";
        break;
      case 1507515133:
        no = "NodeMCU #4";
        break;
      default:
        no = "Nenhum";
        break;
    }
    Serial.printf("Recebi a seguinte mensagem do %u: %s\n", no.c_str(), msg.c_str());
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
    Serial.printf("Delay de %d us\n", from, delay);
}

