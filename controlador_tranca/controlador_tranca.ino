//Armazenamento Interno
//Armazenamento SD Card
//Input do teclado
//Recebimento de dados pelo TCP
//Logica de valicação da tranca
//Relogio Interno
//Eletronica para Liberar a Tranca
//Incrementar com bit shifting and bit masking
//import para acessar memoria interna

//Imports para trabalhar com SD Card
#include <SPI.h>
#include <SD.h>

//Imposts para trabalhar com ethernet e TCP
#include <Ethernet.h>

//Imports para teclado numerico
#include <Keypad.h>

//imports para Relogio interno
#include <Wire.h>
#include <RTClib.h>

//#include <stdlib.h>

RTC_DS3231 rtc; 

/*
int freeMemory() {
  extern int __heap_start, *__brkval;
  int free_mem;
  if ((int)__brkval == 0) {
    free_mem = ((int)&free_mem) - ((int)&__heap_start);
  } else {
    free_mem = ((int)&free_mem) - ((int)__brkval);
  }
  return free_mem;
}
*/

void startRTC(){
  if(!rtc.begin()){
    Serial.println("RTC não encontrado");
    while(1);
  }

  //Serial.println("RTC encontrado");
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

//Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
IPAddress ip(192,168,0,120);                        //IP Local Arduino
IPAddress server(192,168,0,103);                    //IP Servidor Python
int port = 5000;
String incomingData = "";
bool receivingJson = false;
EthernetClient client;

File permFile;


void startEthernet(){
  Ethernet.begin(mac, ip);

  delay(1000);


 if(client.connect(server, port)){
  Serial.println("Conectado ao servidor!");

  while(client.connected()){
    handleEthernetCommunication();
  }
 } else {
  Serial.println("Falha na conexão");
 }

}

void handleDisconnect(){
  if(!client.connected()){
    Serial.println("Desconectado. Reconectando...");
    if(client.connect(server, port)){
      
      Serial.println("Reconectado");
    
      while(client.connected()){
        handleEthernetCommunication();
      }
    } else {
      Serial.println("Falha em reconexão...");
    }
    delay(5000);
  }
}



void handleEthernetCommunication(){

   while(!startFile()){
      delay(1000);
  }

  while(client.available()){
    char c = client.read();

    if(!receivingJson){
      incomingData += c;
      if (incomingData.endsWith("<START>")) {
        incomingData = "";  // Limpa para começar a receber JSON
        receivingJson = true;
      } else if (incomingData.length() > 100) {
        incomingData = "";  // Evita lixo no início
      }
    }
    else { //recebendo JSON 
      incomingData += c;
       if (incomingData.endsWith("<END>")) {
        incomingData.replace("<END>", "");
    
        writeToPermissionFile(incomingData);
        delay(100);
        receivingJson = false;
        incomingData = "";
      }
    }

    
  }

  permFile.close();
}

//SD CARD
const byte sdCardPin = 4;

bool sdBeginResult = false;

void startSDCard(){
  sdBeginResult = SD.begin(sdCardPin);

  if(!sdBeginResult){
    Serial.println("Falha ao inicializar o SD card!");
    return;
  }
   Serial.println("SD card inicializado com sucesso!");


/*  Serial.println(SD.exists("/perm.txt"));

  File file = SD.open("/perm.txt", FILE_WRITE);
  
  if (!file) {
    Serial.println("Erro ao abrir arquivo!");
    return;
  } 

  file.print("Teste de Escrita");

  */
}


bool startFile(){
  while(!sdBeginResult){
    startSDCard();
   delay(1000);
  }

  Serial.println(SD.exists("per.txt"));
  permFile = SD.open("per.txt", FILE_WRITE);

  if (!permFile) {
    Serial.println("Erro ao abrir arquivo!");
    permFile.close();
    return false;
  }

  Serial.println(permFile);
  return true;
}

void writeToPermissionFile(String data) {
  
  if(data == ""){
    return;
  }

  // Divide a string pelos separadores
  int firstComma = data.indexOf(',');
  int index = data.substring(0, firstComma).toInt();
  Serial.println(data);
  
  // Se índice for 0, limpa o arquivo
  if (index == 0) {
    permFile.seek(0); // Posiciona no início
    permFile.print(""); // Limpa o conteúdo
    Serial.println("limpando arquivo");
  }

  // Escreve a linha
  permFile.println(data);
  Serial.println("Dados escritos: ");
 
}

//KEYPAD
const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
  };

byte rowPins[ROWS] = {3, 8, 7, 5}; // Pinos das linhas
byte colPins[COLS] = {4, 2, 6}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins,colPins, ROWS, COLS);

const byte maxDisplayNum = 16;
char displayfeedback[maxDisplayNum] = "";
int feedbackPointer = 0;
const byte pinled = 13;


void callkeypadInput(){
  char key = keypad.getKey();

  if(key){
    handleKey(key);
    Serial.print("Display atual: ");
    Serial.println(displayfeedback);
  }
}

void handleKey(char key){
    if(key == '#'){
      startValidation();
      clearFeedback();
      return;
    }

    if(key == '*'){
      removeLastKey();
      return;
    }

    if((feedbackPointer +1) == maxDisplayNum){
      return;
    }

    displayfeedback[feedbackPointer] = key;
    feedbackPointer++;
}

void removeLastKey(){
  feedbackPointer--;
  displayfeedback[feedbackPointer] = '\0';
}

void startValidation(){
  Serial.println("start validation");
  pinMode(pinled, OUTPUT);
  
  digitalWrite(pinled, HIGH);

  delay(3000);

  digitalWrite(pinled, LOW);
}
void clearFeedback(){
  feedbackPointer = 0;
  memset(displayfeedback, '\0', sizeof(displayfeedback));
}



void setup() {
  Serial.begin(9600);
  Serial.println("Iniciando Arduino");
  //startRTC();
  startSDCard();

  delay(1500);
  startEthernet();
}

void loop() {
  handleDisconnect();
  //callkeypadInput();
}