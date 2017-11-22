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
// 4. Envia mensagem broadcast para os nos da rede mesh;
// 5. Recebe mensagem broadcast dos nos da rede mesh;
// 6. Envia SMS com SIM800L.
//************************************************************************************************************************

//************************************************************************************************************************
// Incluir as bibliotecas:
//************************************************************************************************************************
#include "EmonLib.h"
#include <SPI.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <virtuabotixRTC.h>
#include <TaskScheduler.h>

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

// SCT E LDR:
EnergyMonitor emon1; // Monitor de energia para o sensor de corrente;
double numVoltasBobina = 2000; // Numero de voltas na bobina do leitor de corrente;
double resistorCarga = 22; // Carga do resistor;
#define SELETOR D1 // Pino responsavel por selecionar um dos dois sensores analogicos (0: LDR, 1: SCT);
#define PINO_LED D4 // Pino responsavel por ligar o LED;
#define PINO_MUX A0 // Pino responsavel por ler a entrada analogica selecionada pelo seletor;
int ldrValor = 0; // Valor lido do LDR;
double Irms = 0.00; // Valor lido do leitor corrente;
#define CLARO 700 // Cosiderado claro ate 700 de luminosidade;
#define CORRENTE 0.25 // Media da variacao da corrente para determinar o gasto ou nao;
int estabilizador = 0; // Usado enquanto o sistema inicializa para estabilizar a corrente.

// RTC DS1302:
virtuabotixRTC RTC(D5, D6, D7); // Pinos ligados ao modulo. Parametros: RTC(clock, data, rst)
int ultimoEnvioHora = -1; // Inicializa a hora de envio com valor absurdo
int ultimoEnvioMinutos = -1; // Inicializa o minuto de envio com valor absurdo

// MSG SIM 800L:
String msgDiaClaro = String("Ponto de iluminacao aceso durante dia claro. Endereco...");
String msgDiaEscuro = String("Ponto de iluminacao apagado durante dia escuro. Endereco...");
String msgNoite = String("Ponto sem iluminacao em horario noturno. Endereco...");
String telefone = String("988338506"); // Telefone da empresa que faz manutencao na iluminacao publica

// Escopo da funcao:
void sendMessage();

//************************************************************************************************************************
// Configuracao de tasks - Parametros: (tempo em millis, intervalo de repeticao, callback)
//************************************************************************************************************************
Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );
Task taskReadAmperage( TASK_SECOND * 0.1 , TASK_FOREVER, &readAmperage );
Task taskReadLuminosity( TASK_SECOND * 0.1 , TASK_FOREVER, &readLuminosity );
Task taskReadTime( TASK_SECOND * 0.1 , TASK_FOREVER, &readTime );

/*
Task t1(1, TASK_FOREVER, &readAmperage);   // Tarefa 1: Executa SCT;
Task t2(1, TASK_FOREVER, &readLuminosity); // Tarefa 2: Executa LDR;
Task t3(10, TASK_FOREVER, &readTime);    // Tarefa 3: Executa RTC;
Task t4(1000, TASK_FOREVER, &sendMessage); // Tarefa 4: Comunicacao Mesh.
*/

// Escalonador de tarefas:
// Scheduler runner;

//************************************************************************************************************************
// Configuracao de debug:
//************************************************************************************************************************
#define DEV true // Ativa/desativa o modo desenvolvedor para mostrar os debugs na serial;

void receivedCallback(uint32_t from, String &msg) {
    Serial.print("Mensagem recebida do no ");
    Serial.print(from + ": ");
    Serial.println(msg);
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

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Tempo ajustado: %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

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
    mesh.onNewConnection(&newConnectionCallback); // Chamada da funcao quando um novo no se conecta a rede mesh;
    mesh.onChangedConnections(&changedConnectionCallback); // Chamada da funcao ao mudar de conexao;
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    
    // Add tarefas
    mesh.scheduler.addTask( taskSendMessage );
    taskSendMessage.enable() ;
    
    mesh.scheduler.addTask( taskReadAmperage );
    taskReadAmperage.enable() ;
    
    mesh.scheduler.addTask( taskReadLuminosity );
    taskReadLuminosity.enable() ;
    
    mesh.scheduler.addTask( taskReadTime );
    taskReadTime.enable() ;
  
    // Calibracao do pino
    emon1.current(PINO_MUX, (numVoltasBobina / resistorCarga));

    // Configuraçes iniciais de data e hora - Apos configurar, comentar a linha abaixo;
    //RTC.setDS1302Time(00, 47, 22, 1, 12, 11, 2017); // Formato: (segundos, minutos, hora, dia da semana, dia do mes, mes, ano);
    
    /*
    // Inicializacao do TaskScheduler
    runner.init();
    Serial.println("\nAgendador inicializado.");

    runner.addTask(t1);
    Serial.println("\nTarefa 1 (SCT) add.");

    runner.addTask(t2);
    Serial.println("Tarefa 2 (LDR) add.");

    runner.addTask(t3);
    Serial.println("Tarefa 3 (RTC) add.");

    runner.addTask(t4);
    Serial.println("Tarefa 4 (MESH) add.");
    
    delay(5000);

    t1.enable();
    Serial.println("Tarefa 1 (SCT) habilitada.");
    t2.enable();
    Serial.println("Tarefa 2 (LDR) habilitada.");
    t3.enable();
    Serial.println("Tarefa 3 (RTC) habilitada.");
    t4.enable();
    Serial.println("Tarefa 4 (MESH) habilitada.");
    */
}

//************************************************************************************************************************
// Execucao do programa:
//************************************************************************************************************************
void loop() {
  // Atualizar informacoes da rede mesh:
  mesh.update();
  
  // Executar tarefas:
  // runner.execute();
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

        // Envia uma mensagem persistindo envia mais na hora seguinte.
        // MSG: Alerta de ponto sem iluminacao em horario noturno.
        if(RTC.hours > ultimoEnvioHora){
          requestSMS(msgNoite);
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
      if(ultimoEnvioHora != -1){
        Serial.print("Ultimo envio de SMS: ");
        Serial.print(ultimoEnvioHora);
        Serial.print("h:");
        Serial.print(ultimoEnvioMinutos);
        Serial.print("m\n");
      }
    }
  }

  // Esta em horario considerado dia
  else{

    // Verifica corrente e luminosidade
    if(Irms >= CORRENTE && ldrValor <= CLARO){ // Corrente acima do permitido para o horario e luminosidade indicando que esta claro.

      // Envia a uma mensagem persistindo envia mais na hora seguinte.
      // MSG: Ponto de iluminacao aceso durante o dia.
      if(RTC.hours > ultimoEnvioHora){
        requestSMS(msgDiaClaro);
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
      if(RTC.hours > ultimoEnvioHora){
        requestSMS(msgDiaEscuro);
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
      if(ultimoEnvioHora != -1){
        Serial.print("Ultimo envio de SMS: ");
        Serial.print(ultimoEnvioHora);
        Serial.print("h:");
        Serial.print(ultimoEnvioMinutos);
        Serial.print("m\n");
      }
    }
  }
  
  // Atualiza o tempo entre 1 e 5 segundos:
   taskReadTime.setInterval( random( TASK_SECOND * 0.1, TASK_SECOND * 0.5 ));
}

void readAmperage(){ // Lendo corrente
  changeSensor(HIGH); // Seleciona no mux o leitor de corrente;
  double corrente_lida = emon1.calcIrms(1480);
  
  //Calcula a corrente:
  if(corrente_lida > 0.05 && corrente_lida < CORRENTE) //Filtra outiliers
    Irms = 0.00;
  else
    Irms = corrente_lida;
  
  if(DEV){
    // Mostra o valor da corrente no serial monitor:
    Serial.print("Corrente: ");
    Serial.println(Irms);
  }
  
  // Atualiza o tempo entre 1 e 5 segundos:
   taskReadAmperage.setInterval( random( TASK_SECOND * 0.1, TASK_SECOND * 0.5 ));
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
   
   // Atualiza o tempo entre 1 e 5 segundos:
   taskReadLuminosity.setInterval( random( TASK_SECOND * 0.1, TASK_SECOND * 0.5 ));
}

void requestSMS(String mensagem){ // Requisita o envio de SMS para o no principal.
    // Formato da mensagem enviada: verificador!corrente!luminosidade!hora!no
    // Ex.:  V!0.20!325!18:30!NodeMCU#1
    String no = String(mesh.getNodeId());
    String corrente = String(Irms);
    String luminosidade = String(ldrValor);
    String hora = String(RTC.hours);
    String msg = String("V!" + corrente + "!" + luminosidade + "!" + hora + "!" + no + "!" + mensagem + "\n\n");
    Serial.print("\nNo ");
    Serial.print(mesh.getNodeId());
    Serial.print(" requisitando envio de SMS ao no principal com a mensagem: ");
    Serial.println(msg);

    // Atualiza a hora e minuto do ultimo envio:
    ultimoEnvioHora = RTC.hours;
    ultimoEnvioMinutos = RTC.minutes;
    
    
    // Envia para todos os nos da rede mesh.
    mesh.sendBroadcast(msg);
}

//************************************************************************************************************************
// Funcoes da rede Mesh:
//************************************************************************************************************************
void sendMessage(){ // Enviando mensagens
    // Formato da mensagem enviada: corrente!luminosidade!hora!no
    // Ex.:  0.20!325!18:30!NodeMCU#1
    String no = String(mesh.getNodeId());
    String corrente = String(Irms);
    String luminosidade = String(ldrValor);
    String hora = String(RTC.hours);
    String msg = String("F!" + corrente + "!" + luminosidade + "!" + hora + "!" + no + "\n\n");

    Serial.print("\nEnviando status do no ");
    Serial.print(mesh.getNodeId());
    Serial.print(" para todos os nos da rede com a seguinte mensagem: ");
    Serial.println(msg);

    // Envia para todos os nos da rede mesh.
    mesh.sendBroadcast(msg);
    
    // Atualiza o tempo entre 1 e 5 segundos:
    taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}
