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
//************************************************************************************************************************

//************************************************************************************************************************
// Incluir as bibliotecas:
//************************************************************************************************************************
#include "EmonLib.h"
#include <SPI.h>

//************************************************************************************************************************
// Declaracao de variaveis e Constantes:
//************************************************************************************************************************
// SCT E LDR:
EnergyMonitor emon1; // Monitor de energia para o sensor de corrente;
double numVoltasBobina = 2000; // Numero de voltas na bobina do leitor de corrente;
double resistorCarga = 22; // Carga do resistor;
#define PINO_LED 7 // Pino responsavel por ligar o LED;
#define PINO_LDR A0 // Porta RX como GPIO analgica;
#define PINO_SCT A5 // Pino responsavel por ler a entrada analogica selecionada pelo seletor;
int ldrValor = 0; //Valor lido do LDR;

//************************************************************************************************************************
// Configuracao do programa:
//************************************************************************************************************************
void setup() {
    Serial.begin(9600); //SERIAL_8N1, SERIAL_TX_ONLY
    pinMode(PINO_LED, OUTPUT); // Define o pino do led como saida
       
    // Calibracao do pino
    emon1.current(PINO_SCT, (numVoltasBobina / resistorCarga));
}

//************************************************************************************************************************
// Execucao do programa:
//************************************************************************************************************************
void loop() {       
    // Ler corrente:
    readAmperage();
    
    // TODO: Ler horario
    
    // Ler luminosidade:
    readLuminosity();
     
    // TODO: Enviar mensagem.
    //sendMessage();
    
    // TODO: Usar #include <TaskScheduler.h> para dormir por 30 minutos.
}

//************************************************************************************************************************
// Funcoes pessoais:
//************************************************************************************************************************

void readAmperage(){ // Lendo corrente
  double Irms = emon1.calcIrms(1480); //Calcula a corrente

  // Mostra o valor da corrente no serial monitor:
  Serial.print("\nCorrente: ");
  Serial.println(Irms);
  delay(1000);
}

void readLuminosity(){ // Lendo luminosidade
   ldrValor = analogRead(PINO_LDR); // O valor lido será entre 0 e 1023   
   
   digitalWrite(PINO_LED, HIGH);
   
   if(ldrValor >= 700){digitalWrite(PINO_LED, HIGH);}
   else{digitalWrite(PINO_LED, LOW);}
   
   // Imprime o valor lido do LDR no monitor serial:
   Serial.print("\nLuminosidade: ");
   Serial.println(ldrValor);
   delay(1000);
}
