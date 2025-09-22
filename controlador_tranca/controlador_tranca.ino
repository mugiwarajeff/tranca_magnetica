//Armazenamento Interno - OK
//Armazenamento SD Card - OK
//Input do teclado - OK
//Recebimento de dados pelo TCP - OK / Discussão com professor Camilo, atualizaçãao nao pode ser feita em periodo de aula
//Lembrar de colocar no artigo problemas relacionados a variaveis e problemas de memoria
//
//Relogio Interno - OK
//Logica de valicação da tranca 
//Eletronica para Liberar a Tranca


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

RTC_DS3231 rtc; 


//Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
IPAddress ip(192,168,0,124);                        //IP Local Arduino
IPAddress server(192,168,0,121);                    //IP Servidor Python
int port = 5000;
EthernetClient client;

// Variáveis para receber dados do servidor
char incomingBuffer[64];
short dataPointer = 0;
const char startTag[] = "<START>";
const char endTag[] = "<END>";
const int startTagLen = strlen(startTag);
const int endTagLen = strlen(endTag);


//SD CARD
const byte sdCardPin = 4;
bool sdBeginResult = false;
const char* permissionFile = "per.txt";
File permFile; // Variável global para o arquivo de permissões

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
short feedbackPointer = 0;
const byte pinled = 13;


void startRTC(){
  if(!rtc.begin()){
    Serial.println("RTC não encontrado");
    while(1);
  }

  //Serial.println("RTC encontrado");
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}


void startEthernet(){
  Serial.println("Iniciando Ethernet...");
  Ethernet.begin(mac, ip);
  delay(1000);
  if(client.connect(server, port)){
    Serial.println("Conectado ao servidor!");
  } else {
    Serial.println("Falha na conexao");
  }
}

void handleDisconnect(){
  if(!client.connected()){
    Serial.println("Desconectado. Reconectando...");
    client.stop();
    delay(1000);
    startEthernet();
  }
}

void processAndSavePacket(char* data) {
    char* commaPtr = strchr(data, ',');

    if (commaPtr == NULL) {
      Serial.println("Formato de dados invalido.");
      return;
    }

    // Extrai o índice
    *commaPtr = '\0'; // Temporariamente termina a string no local da vírgula
    int index = atoi(data);
    *commaPtr = ','; // Restaura a vírgula

    Serial.println(data);
     Serial.println(index);

 // Se o índice for 0, apaga o arquivo e reabre em modo de escrita
    
    if (index == 0) {
      if (SD.exists(permissionFile)) {
        SD.remove(permissionFile);
        Serial.println("Arquivo per.txt apagado.");
      }
      
      // Fecha o arquivo se ele estiver aberto para reabrir em modo de escrita

   
      if(permFile){
        permFile.close();
      }
      permFile = SD.open(permissionFile, FILE_WRITE);
      
    }  else {
      // Abre o arquivo em modo de append se não estiver aberto
      if(!permFile){
         permFile = SD.open(permissionFile, FILE_WRITE);
      }
    }

    
    
    if (permFile) {
      permFile.println(data);
      permFile.flush(); // Garante que os dados sejam gravados no SD
      Serial.println("Dados escritos no arquivo.");
    } else {
      Serial.println("Erro ao abrir/escrever no arquivo!");
    }
}


void handleEthernetCommunication(){
  
  // Lê apenas um byte por vez
  while (client.available()) {
    char c = client.read();

    // Adiciona o caractere ao buffer
    if (dataPointer < sizeof(incomingBuffer) - 1) {
      incomingBuffer[dataPointer++] = c;
      incomingBuffer[dataPointer] = '\0'; // Termina a C-string
    }

    // Procura por <START> e <END> no buffer usando C-strings
    char* startPtr = strstr(incomingBuffer, startTag);
    char* endPtr = strstr(incomingBuffer, endTag);
    // Se um pacote completo foi encontrado
    if (startPtr != NULL && endPtr != NULL && endPtr > startPtr) {
      // Extrai a mensagem completa sem as tags
      *endPtr = '\0';
      char* payloadPtr = startPtr + startTagLen;

      Serial.print("Pacote completo recebido: ");
      Serial.println(payloadPtr);
      
      // Processa e salva o pacote
      processAndSavePacket(payloadPtr);
      
      // Envia o ACK de volta para o servidor
      client.write("<ACK>");
      Serial.println("ACK enviado ao servidor.");
      
      // Limpa o buffer para a próxima mensagem, movendo os dados restantes para o início
      int remainingLen = strlen(endPtr + endTagLen);
      memmove(incomingBuffer, endPtr + endTagLen, remainingLen + 1);
      dataPointer = remainingLen;
    }
    
    // Limpeza de buffer de segurança para evitar estouro de memória
    if(dataPointer >= sizeof(incomingBuffer) -1){
        Serial.println("Buffer cheio, limpando...");
        memset(incomingBuffer, 0, sizeof(incomingBuffer));
        dataPointer = 0;
    }
  } 
}

void startSDCard(){
  sdBeginResult = SD.begin(sdCardPin);

  if(!sdBeginResult){
    Serial.println("Falha ao inicializar o SD card!");
    return;
  }
   Serial.println("SD card inicializado com sucesso!");
}



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

  startSDCard();
  //startRTC();
  delay(1500);
  startEthernet();
 
}

void loop() {
  handleEthernetCommunication();
  handleDisconnect();
  //callkeypadInput();
}