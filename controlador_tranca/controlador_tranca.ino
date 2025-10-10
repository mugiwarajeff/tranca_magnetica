//Armazenamento Interno - OK
//Armazenamento SD Card - OK
//Input do teclado - OK
//Recebimento de dados pelo TCP - OK / Discussão com professor Camilo, atualizaçãao nao pode ser feita em periodo de aula
//Lembrar de colocar no artigo problemas relacionados a variaveis e problemas de memoria
//
//Relogio Interno - OK
//Logica de valicação da tranca 
//Eletronica para Liberar a Tranca

//#define DEBUG_TCP
//#define DEBUG_PIN_VALIDATION
//#define DEBUG_RTC
#define RTC_ON
//#define TCP_ON
#define KEYBOARD_ON
#define SDCARD_ON

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

byte currentHour;   
byte currentMinute; 
byte currentDay;    
byte currentMonth;  
short currentYear;


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
const char cleanTag[] = "<CLEAN>";
const int startTagLen = strlen(startTag);
const int endTagLen = strlen(endTag);
const int cleanTagLen = strlen(cleanTag); 


//SD CARD
const byte sdCardPin = 4;
bool sdBeginResult = false;
const char* permissionFile = "per.txt";
File permFile; // Variável global para o arquivo de permissões

//lock
const byte lockPin = 10; 

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
byte colPins[COLS] = {9, 2, 6}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins,colPins, ROWS, COLS);
const byte maxDisplayNum = 7;
char displayfeedback[maxDisplayNum] = "";
short feedbackPointer = 0;

void activateLock() {
    // 1. Aciona o Módulo MOS (sinal HIGH - 5V - liga a chave de 12V)
    digitalWrite(lockPin, HIGH);
    Serial.println("Ativando tranca");

    // 2. Mantém a tranca acionada por 1 segundo (Pulso de segurança)
    // ATENÇÃO: Este pulso deve ser CURTO (1000ms = 1 segundo). Se for muito longo, 
    // a tranca eletromagnética (solenoide) pode superaquecer e queimar.
    delay(1000); 
    // 3. Desliga o Módulo MOS (sinal LOW - 0V - desliga a chave de 12V)
    digitalWrite(lockPin, LOW);
      Serial.println("Desativando tranca");
}


void startRTC(){
  if(!rtc.begin()){
    Serial.println("RTC não encontrado");
    while(1);
  }

  Serial.println("RTC encontrado");
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void updateCurrentTime() {
  // Pega o objeto DateTime atual do RTC
  DateTime now = rtc.now();

  // Mapeia os valores do objeto para as variaveis globais
  currentHour = now.hour();
  currentMinute = now.minute();
  currentDay = now.day();
  currentMonth = now.month();
  currentYear = now.year() % 100; 

  #ifdef DEBUG_RTC
  // DEBUG: Confirma que a leitura do RTC está funcionando
  Serial.print("RTC: ");
  Serial.print(currentDay); Serial.print("/"); 
  Serial.print(currentMonth); Serial.print("/"); 
  Serial.print(currentYear); Serial.print(" ");
  Serial.print(currentHour); Serial.print(":"); 
  Serial.println(currentMinute);
  #endif
}



void startEthernet(){
  Serial.println("Iniciando Ethernet...");
  Ethernet.begin(mac, ip);
  delay(1000);
  if(client.connect(server, port)){
    #ifdef DEBUG_TCP
    Serial.println("Conectado ao servidor!");
    #endif

  } else {
    #ifdef DEBUG_TCP
    Serial.println("Falha na conexao");
    #endif
    
  }
}

void handleDisconnect(){
  if(!client.connected()){
    #ifdef DEBUG_TCP
    Serial.println("Desconectado. Reconectando...");
    #endif
    client.stop();
    delay(1000);
    startEthernet();
  }
}

void processAndSavePacket(char* data) {
    // Se a string de entrada for a tag <CLEAN>, execute a limpeza
    if (strcmp(data, cleanTag) == 0) {
        if (SD.exists(permissionFile)) {
            SD.remove(permissionFile);
            #ifdef DEBUG_TCP
            Serial.println("Arquivo per.txt apagado.");
            #endif
          
        }
        
        // Fecha o arquivo se ele estiver aberto
        if (permFile) {
            permFile.close();
        }
        
        return;
    }
    
    // --- LÓGICA DE SALVAR DADOS (Novo formato sem 'index') ---
    
    // CORRIGIDO: Verifica se o objeto File é válido. 
    // Se não for válido (fechado ou falhou ao abrir), tenta reabri-lo.
    if (!permFile) { 
        permFile = SD.open(permissionFile, FILE_WRITE);
    }
    
    if (permFile) {
      // O data agora é o payload real (ex: "100001,0,0,2,0,26,11,24")
      permFile.println(data);
      permFile.flush(); // Garante que os dados sejam gravados no SD

      #ifdef DEBUG_TCP
      Serial.println(data);
      #endif

      
    } else {

        #ifdef DEBUG_TCP
        Serial.println("ERRO: Falha ao abrir/escrever no arquivo de permissoes.");
        #endif
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

    char* cleanPtr = strstr(incomingBuffer, cleanTag);
    if (cleanPtr != NULL) {
    
        processAndSavePacket(cleanTag); 
        short cleanStartIndex = cleanPtr - incomingBuffer;
        short remainingLen = dataPointer - (cleanStartIndex + cleanTagLen);
        
        memmove(incomingBuffer, cleanPtr + cleanTagLen, remainingLen + 1);
        dataPointer = remainingLen;
        // Volta para o topo do loop para reavaliar o buffer (que agora pode comecar com <START>)
        continue;
    }


    // Procura por <START> e <END> no buffer usando C-strings
    char* startPtr = strstr(incomingBuffer, startTag);
    char* endPtr = strstr(incomingBuffer, endTag);
    
    // Se um pacote completo foi encontrado
    if (startPtr != NULL && endPtr != NULL && endPtr > startPtr) {
      
      // Extrai a mensagem completa sem as tags <START> e <END>
      *endPtr = '\0';
      char* payloadPtr = startPtr + startTagLen;

      processAndSavePacket(payloadPtr);
      client.write("<ACK>");
      
      // Limpa o buffer para a próxima mensagem, movendo os dados restantes para o início
      int remainingLen = strlen(endPtr + endTagLen);
      memmove(incomingBuffer, endPtr + endTagLen, remainingLen + 1);
      dataPointer = remainingLen;


    }
    
    // Limpeza de buffer de segurança para evitar estouro de memória
    if(dataPointer >= sizeof(incomingBuffer) -1){
        memset(incomingBuffer, 0, sizeof(incomingBuffer));
        dataPointer = 0;
    }
  } 
}

bool validatePin(const char* pin) {
    
    // CRÍTICO: Se houver dados de rede, interrompa a validacao para priorizar
    // a comunicação (limpeza e envio de novos dados).
    if (client.available() > 0) {
        #ifdef DEBUG_PIN_VALIDATION
        Serial.println("DEBUG: Rede ativa. Pulando validacao de PIN.");
        #endif
        // Retorna FALSE para negar o acesso enquanto a rede está ocupada.
        return false;
    }

    bool validationResult = false;
    
    #ifdef DEBUG_PIN_VALIDATION
    // DEBUG: Confirma o PIN de entrada antes de começar
    Serial.print("DEBUG: PIN de entrada (6 digitos) para validacao: ");
    Serial.println(pin);
    #endif
    
    // 1. Fecha o permFile se estiver aberto (de FILE_WRITE)
    if (permFile) {
        permFile.close();
    }
    
    // 2. Abre o arquivo para leitura
    permFile = SD.open(permissionFile, FILE_READ);
    
    if (!permFile) {
      
        #ifdef DEBUG_PIN_VALIDATION
        Serial.println("DEBUG: Erro ao abrir arquivo para leitura.");
        #endif
        return false;
    }
    
    char lineBuffer[64];
    short bufferPointer = 0;
    
    while (permFile.available()) {
        char c = permFile.read();

        if (c == '\n' || c == '\r' || !permFile.available()) {
            
            // Se for o final do arquivo e o bufferPointer > 0, processa a ultima linha.
            if (!permFile.available() && c != '\n' && c != '\r') {
                lineBuffer[bufferPointer++] = c;
            }

            lineBuffer[bufferPointer] = '\0'; 
            
            #ifdef DEBUG_PIN_VALIDATION
            Serial.print("DEBUG: Linha lida: ");
            Serial.println(lineBuffer);
            #endif

            // --- INICIO DA EXTRAÇÃO E VALIDAÇÃO ---
            
            // Fazemos uma cópia da linha, pois strtok altera a string original
            char tempLine[64];
            strncpy(tempLine, lineBuffer, sizeof(tempLine) - 1);
            tempLine[sizeof(tempLine) - 1] = '\0';

            // 1. EXTRAI E VALIDA O PIN
            char* storedPinStr = strtok(tempLine, ",");
            
            if (storedPinStr) {
                
                // Valida o PIN (1o campo)
                if (strcmp(storedPinStr, pin) == 0) {
                     
                    #ifdef DEBUG_PIN_VALIDATION
                    Serial.println("DEBUG: PIN Encontrado. Verificando Horário.");
                    #endif
                    
                    // 2. EXTRAI OS CAMPOS DE TEMPO E DATA
                    // startHour
                    byte startHour = atoi(strtok(NULL, ","));
                    // startMinute
                    byte startMinute = atoi(strtok(NULL, ","));
                    // endHour
                    byte endHour = atoi(strtok(NULL, ","));
                    // endMinute
                    byte endMinute = atoi(strtok(NULL, ","));
                    // day
                    byte savedDay = atoi(strtok(NULL, ","));
                    // month
                    byte savedMonth = atoi(strtok(NULL, ","));
                    // year
                    byte savedYear = atoi(strtok(NULL, ","));
                    
                    
                    // --- VALIDAÇÃO DE DATA E HORA ---
                    
                    // A) VALIDAÇÃO DE DATA (TEM QUE SER O DIA E ANO EXATOS)
                    bool dateMatch = (currentYear == savedYear) && 
                                     (currentMonth == savedMonth) && 
                                     (currentDay == savedDay);
                    
                    if (dateMatch) {
                        
                        // B) VALIDAÇÃO DE HORA
                        long currentTotalMins = currentHour * 60L + currentMinute;
                        long startTotalMins = startHour * 60L + startMinute;
                        long endTotalMins = endHour * 60L + endMinute;

                        bool timeMatch = false;

                        // Se endHour for menor que startHour (ex: 22h-0h)
                        if (endHour <= startHour) {
                            // Slot que cruza meia-noite (22h:00 - 00:00)
                            // Acesso é permitido se:
                            // 1. Está após a hora de início (startHour até 23:59) OU
                            // 2. Está antes da hora de término (00:00 até endHour)
                            
                            // Para o seu caso (22h-0h): endTotalMins sera 0.
                            if (currentHour >= startHour || currentHour < endHour) {
                                timeMatch = true;
                            }
                        } else {
                            // Slot de tempo normal (ex: 10h-12h)
                            timeMatch = (currentTotalMins >= startTotalMins) && 
                                        (currentTotalMins < endTotalMins);
                        }

                        if (timeMatch) {
                            // PIN VALIDADO E DENTRO DO TEMPO!
                            validationResult = true; 
                  
                            #ifdef DEBUG_PIN_VALIDATION
                            Serial.println("DEBUG: TEMPO E PIN SUCESSO! Acesso Concedido.");
                            #endif
                            break; // Sai do loop principal
                        } else {
                          
                            #ifdef DEBUG_PIN_VALIDATION
                            Serial.print("DEBUG: PIN OK, mas FORA DO TEMPO.");
                            #endif
                        }
                    } else {
                    
                        #ifdef DEBUG_PIN_VALIDATION
                        Serial.println("DEBUG: PIN OK, mas FORA DA DATA. Continuando a busca.");
                        #endif
                    }
                }
            }
            // --- FIM DA EXTRAÇÃO E VALIDAÇÃO ---

            bufferPointer = 0;
            memset(lineBuffer, 0, sizeof(lineBuffer));
        } else {
            if (bufferPointer < sizeof(lineBuffer) - 1) {
                lineBuffer[bufferPointer++] = c;
            } 
        }
    }
    
    // 3. Fecha o arquivo de leitura
    if (permFile) {
        permFile.close();
    }
    
    // 4. Reabre o arquivo para escrita/append
    permFile = SD.open(permissionFile, FILE_WRITE);

    #ifdef DEBUG_PIN_VALIDATION
    if (validationResult) {
        Serial.println("DEBUG: RESULTADO FINAL: PIN VALIDADO COM SUCESSO!");
    } else {
        Serial.println("DEBUG: RESULTADO FINAL: FALHA NA VALIDACAO.");
    }
    #endif
    
    return validationResult;
}

void startSDCard(){
  sdBeginResult = SD.begin(sdCardPin);

  if(!sdBeginResult){
     Serial.println("Falha ao Iniciar SD Card");
    return;
  }

  Serial.println("Sucesso ao Iniciar SD Card");
}



void callkeypadInput(){
  char key = keypad.getKey();

  if(key){
    handleKey(key);
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
  updateCurrentTime();
  bool result =validatePin(displayfeedback);
  Serial.println(result);
  //pinMode(pinled, OUTPUT);
  //digitalWrite(pinled, HIGH);
  //delay(3000);
  //digitalWrite(pinled, LOW);
}

void clearFeedback(){
  feedbackPointer = 0;
  memset(displayfeedback, '\0', sizeof(displayfeedback));
}

void setup() {
  Serial.begin(9600);
  pinMode(lockPin, OUTPUT);
  digitalWrite(lockPin, LOW);

  #ifdef SDCARD_ON
  startSDCard();
  #endif

  #ifdef RTC_ON
  startRTC();
  #endif

  delay(1500);

  #ifdef TCP_ON
  startEthernet();
  #endif
}

void loop() {
  #ifdef TCP_ON
  handleEthernetCommunication();
  handleDisconnect();
  #endif

  delay(5000);
  activateLock();


  #ifdef KEYBOARD_ON
  callkeypadInput();
  #endif
}