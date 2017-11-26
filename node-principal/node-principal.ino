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
// 1. Le sensor de luminosidade ldr;
// 2. Envia mensagem broadcast para os nos da rede mesh;
// 3. Recebe mensagem broadcast dos nos da rede mesh;
// 4. Envia SMS com SIM800L.
//************************************************************************************************************************

//************************************************************************************************************************
// Incluir as bibliotecas:
//************************************************************************************************************************
#include <SPI.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>
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

// LDR:
#define PINO_LED D4 // Pino responsavel por ligar o LED;
#define PINO_MUX A0 // Pino responsavel por ler a entrada analogica selecionada pelo seletor;
int ldrValor = 0; // Valor lido do LDR;
#define CLARO 700 // Cosiderado claro ate 700 de luminosidade;

// MSG SIM 800L:
String msgDiaClaro = String("Ponto de iluminacao aceso durante dia claro. Endereco...");
String msgDiaEscuro = String("Ponto de iluminacao apagado durante dia escuro. Endereco...");
String msgNoite = String("Ponto sem iluminacao em horario noturno. Endereco...");
#define TELEFONE "988338506" //= String("999058910"); // Telefone da empresa que faz manutencao na iluminacao publica: 988338506

// Escopo da funcao:
void sendMessage();
void sendSMS();

//************************************************************************************************************************
// Configuracao de tasks - Parametros: (tempo em millis, intervalo de repeticao, callback)
//************************************************************************************************************************
Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );
Task taskReadLuminosity( TASK_SECOND * 1 , TASK_FOREVER, &readLuminosity );

//************************************************************************************************************************
// Configuracao de debug:
//************************************************************************************************************************
#define DEV false // Ativa/desativa o modo desenvolvedor para mostrar os debugs na serial;


//************************************************************************************************************************
// Funcoes callback:
//************************************************************************************************************************
void receivedCallback(uint32_t from, String &msg) {
    Serial.print("Mensagem recebida do no: ");
    Serial.print(from + ": ");
    Serial.println(msg);

    // Desmembrando a String recebida:
    String estado = msg.substring(0, 1);
    String correnteRecebida = msg.substring(2, 6);
    String msgRaw = msg.substring(7, 10);
    int luminosidadeRecebida = atoi(msgRaw.c_str());
    String horaRecebida = msg.substring(11, 12);
    String idNoRecebido = msg.substring(13, 23);
   
    if (estado == "V") { //Se V verifica a luminosidade
      
       // Verifica a luminosidade se esta de acordo com a do proprio no        
       if( ((luminosidadeRecebida <= CLARO) && (ldrValor <= CLARO)) || ((luminosidadeRecebida >= CLARO) && (ldrValor >= CLARO)) ){  
         
         Serial.print("Luminosidade recebida do no receptor/emissor: ");
         Serial.print(luminosidadeRecebida);
         Serial.print("\nLuminosidade do no principal: ");
         Serial.print(ldrValor);
         Serial.println();
         
         // Identifica o no e envia SMS.
         String msgRecebida = msg.substring(24);
         String msgEnviar = String("No " + idNoRecebido + ". " + msgRecebida);
         
         if(DEV){
           Serial.print("\nEnviar SMS. Mensagem Recebida: ");
           Serial.println("No " + idNoRecebido + ". " + msgRecebida);
         }
         
         sendSMS(TELEFONE, msgEnviar);
         delay(1000);
       }
       else{
         Serial.print("Nao enviar SMS. Diferenca de luminosidade. No principal: ");
         Serial.print(luminosidadeRecebida);
         Serial.print(". No emissor-receptor: ");
         Serial.print(ldrValor);
         Serial.println();
       }
    }else{
      Serial.print("Nao enviar SMS. Verificador: ");
      Serial.println(estado);
    }
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
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

//************************************************************************************************************************
// Configuracao do programa:
//************************************************************************************************************************
void setup() {
    Serial.begin(9600); // Velocidade de comunicacao;
    
    // Configura o sistema de envio de SMS:
    Serial.print("AT+CMGF=1\n;AT+CNMI=2,2,0,0,0\n;ATX4\n;AT+COLP=1\n"); 
    Serial.println("\nSistema de envio de SMS configurado.");
    
    pinMode(PINO_LED, OUTPUT); // Define o pino do led como saida

    //Mensagens de Debug (tem que ser configurado antes da inicializacao da mesh para ver mensagens de erro de inicializacao, se ocorrerem):
    mesh.setDebugMsgTypes(ERROR | DEBUG);  // De todos tipos opcionais, foram escolhidas somete essas duas.

    // Inicializacao da rede mesh:
    mesh.init(MESH_SSID, MESH_PASSWORD, MESH_PORT); // Configura a rede mesh com os dados da rede sem fio;
    mesh.onReceive(&receivedCallback); // Chamada da funcao ao receber uma mensagem;
    mesh.onNewConnection(&newConnectionCallback); // Chamada da funcao quando um novo no se conecta a rede mesh;
    mesh.onChangedConnections(&changedConnectionCallback); // Chamada da funcao ao mudar de conexao;
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    
    mesh.scheduler.addTask( taskSendMessage );
    taskSendMessage.enable() ;
    
    mesh.scheduler.addTask( taskReadLuminosity );
    taskReadLuminosity.enable() ;
}

//************************************************************************************************************************
// Execucao do programa:
//************************************************************************************************************************
void loop() {
  // Atualizar informacoes da rede mesh:
  mesh.update();
 }

//************************************************************************************************************************
// Funcoes pessoais:
//************************************************************************************************************************
void readLuminosity(){ // Lendo luminosidade
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
     Serial.print("Luminosidade: ");
     Serial.println(ldrValor);
   }
   
   // Ajusta o tempo de 1 a 5 segundos:
   taskReadLuminosity.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

void sendMessage(){ // Enviando mensagens
    // Formato da mensagem enviada: corrente!luminosidade!hora!no
    // Ex.:  0.20!325!18:30!NodeMCU#1
    String no = String(mesh.getNodeId());
    String corrente = String("0.00");
    String luminosidade = String(ldrValor);
    String hora = String("0:00");
    String msg = String("F!" + corrente + "!" + luminosidade + "!" + hora + "!" + no + "\n\n");

    if(DEV){
      Serial.print("\nEnviando status do no ");
      Serial.print(mesh.getNodeId());
      Serial.print(" para todos os nos da rede com a seguinte mensagem: ");
      Serial.println(msg);
    }

    // Envia para todos os nos da rede mesh.
    mesh.sendBroadcast(msg);
    
    // Ajusta o tempo de 1 a 5 segundos:
    taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

void sendSMS(String numeroSMS, String mensagem){ // Envia SMS para o numero cadastrado.
    // Configura o sistema de envio de SMS:
    Serial.print("AT+CMGF=1\n;AT+CNMI=2,2,0,0,0\n;ATX4\n;AT+COLP=1\n"); 
    Serial.println("\nSistema de envio de SMS configurado.");
    Serial.print("AT+CMGS=\"" + numeroSMS + "\"\n");
    Serial.print(mensagem + "\n");
    Serial.print((char)26);
}
