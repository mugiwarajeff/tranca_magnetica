//Armazenamento Interno - OK
//Armazenamento SD Card - OK
//Input do teclado - OK
//Recebimento de dados pelo TCP - OK / Discussão com professor Camilo, atualizaçãao nao pode ser feita em periodo de aula
//Lembrar de colocar no artigo problemas relacionados a variaveis e problemas de memoria
//Relogio Interno - OK
//Logica de valicação da tranca - OK
//Eletronica para Liberar a Tranca - OK

//#define DEBUG_TCP
//#define DEBUG_PIN_VALIDATION
//#define DEBUG_RTC
//#define DEBUG_LOCK
#define RTC_ON
#define TCP_ON
#define KEYBOARD_ON
#define SDCARD_ON
#define LOCK_ON
#define LCD_ON
//#define SCHEDULEDMODE

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

//Import para display LCD
#include <LiquidCrystal_I2C.h> //instalar  LiquidCrystal I2C
#include <string.h>

RTC_DS3231 rtc; 

byte currentHour;   
byte currentMinute; 
byte currentDay;    
byte currentMonth;  
byte currentYear;

//LCD Controll
LiquidCrystal_I2C lcd(0x27, 16, 1);

#ifdef SCHEDULEDMODE
    bool hasExecutedToday = false; 
    const byte TARGET_HOUR = 11;
#endif

 
//Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
IPAddress ip(192,168,0,110);                        //IP Local Arduino
IPAddress server(192,168,0,106);   //192.168.0.106             //IP Servidor Python
short port = 5000;
EthernetClient client;

// Variáveis para receber dados do servidor
char incomingBuffer[38];
short dataPointer = 0;
const char startTag[] PROGMEM = "<START>"; //lembrar de colocar planilha de solução
const char endTag[] PROGMEM = "<END>";
const char cleanTag[] PROGMEM = "<CLEAN>";
const char finalTag[] PROGMEM = "<ENDTX>";
const byte startTagLen = strlen_P(startTag);
const byte endTagLen = strlen_P(endTag);
const byte cleanTagLen = strlen_P(cleanTag); 
const byte finalTagLen = strlen_P(finalTag);


//SD CARD
const byte sdCardPin = 4;

bool sdBeginResult = false;
const char permissionFile[] PROGMEM = "per.txt";
const byte permissionFileLen = strlen_P(permissionFile);
File permFile; // Variável global para o arquivo de permissões

//lock
const byte lockPin = A1; 

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

    #ifdef DEBUG_LOCK
    Serial.println(F("Ativando tranca"));
    #endif

    // 2. Mantém a tranca acionada por 1 segundo (Pulso de segurança)
    // ATENÇÃO: Este pulso deve ser CURTO (1000ms = 1 segundo). Se for muito longo, a solenoide pode aquecer e queimar

    delay(1000); 
    // 3. Desliga o Módulo MOS (sinal LOW - 0V - desliga a chave de 12V)
    digitalWrite(lockPin, LOW);
    #ifdef DEBUG_LOCK
    Serial.println(F("Desativando tranca"));
    #endif
      
}

void startLCD(){

  Serial.println(F("Iniciando LCD"));
  lcd.init();      // Inicializa o hardware do PCF8574
  lcd.backlight(); // Liga a luz de fundo
  lcd.clear();
  lcd.setCursor(1,0); //linha 1 coluna 1
  
  lcd.print(F("Sistema Iniciando..."));
  delay(1500);
  lcd.clear();
  lcd.print(F("Pin: "));
}

void startRTC(){
  if(!rtc.begin()){
    Serial.println(F("RTC não encontrado"));
    while(1);
  }

  Serial.println(F("RTC encontrado"));
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

  
  #ifdef SCHEDULEDMODE
     if (currentHour > TARGET_HOUR && hasExecutedToday) {
      hasExecutedToday = false;
    }
  #endif


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


void startNetworkSetup(){
  Serial.println(F("Iniciando Ethernet..."));
  Ethernet.begin(mac, ip);
  delay(1000);
 
  #ifdef DEBUG_TCP
  Serial.print(F("IP Local: "));
  Serial.println(Ethernet.localIP());
  #endif
}

bool connectToServer(){

 if(client.connected()) return true; 
 
 #ifdef DEBUG_TCP
 Serial.print(F("Tentando conectar a "));
 Serial.print(server);
 Serial.print(F(" na porta "));
 Serial.println(port);
 #endif

  if(client.connect(server, port)){
    #ifdef DEBUG_TCP
    Serial.println(F("Conectado ao servidor!"));
    #endif
    return true;
  } else {
    #ifdef DEBUG_TCP
    Serial.println(F("Falha na conexao"));
    #endif
    return false;
  }
}

void handleDisconnect(){
  if(!client.connected()){
    #ifdef DEBUG_TCP
    Serial.println(F("Desconectado. Reconectando..."));
    #endif
    client.stop();
    delay(1000);
    connectToServer();
  }
}

void processAndSavePacket(char* data) {
    // Se a string de entrada for a tag <CLEAN>, execute a limpeza
    char cleanTagSRAM[cleanTagLen + 1];
    strcpy_P(cleanTagSRAM, cleanTag);
    char permissionFileSRAM[permissionFileLen + 1];
    strcpy_P(permissionFileSRAM, permissionFile);


    if (strcmp(data, cleanTagSRAM) == 0) {
        if (SD.exists(permissionFileSRAM)) {
            SD.remove(permissionFileSRAM);
            #ifdef DEBUG_TCP
            Serial.println(F("Arquivo per.txt apagado."));
            #endif
          
        }
        
        // Fecha o arquivo se ele estiver aberto
        if (permFile) {
            permFile.close();
        }
        
        return;
    }
    
    
    // Se não for válido (fechado ou falhou ao abrir), tenta reabri-lo.
    if (!permFile) { 
        permFile = SD.open(permissionFileSRAM, FILE_WRITE);
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
        Serial.println(F("ERRO: Falha ao abrir/escrever no arquivo de permissoes."));
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

    char cleanTagSRAM[cleanTagLen + 1];
    strcpy_P(cleanTagSRAM, cleanTag);

    char* cleanPtr = strstr(incomingBuffer, cleanTagSRAM);
    if (cleanPtr != NULL) {
    
        processAndSavePacket(cleanTagSRAM); 
        short cleanStartIndex = cleanPtr - incomingBuffer;
        short remainingLen = dataPointer - (cleanStartIndex + cleanTagLen);
        
        memmove(incomingBuffer, cleanPtr + cleanTagLen, remainingLen + 1);
        dataPointer = remainingLen;
        // Volta para o topo do loop para reavaliar o buffer (que agora pode comecar com <START>)
        continue;
    }

    char finalTagSRAM[finalTagLen + 1];
    strcpy_P(finalTagSRAM, finalTag);

    char* finalPtr = strstr(incomingBuffer, finalTagSRAM);
    if (finalPtr != NULL) {
        // Tag de finalização encontrada! Executa a lógica de parada requerida.
        #ifdef SCHEDULEDMODE
          hasExecutedToday = true;
          client.stop();
        #endif
        
        // Opcional: Limpa o buffer para evitar processamento acidental
        dataPointer = 0;
        incomingBuffer[0] = '\0';

          #ifdef DEBUG_TCP
          Serial.print(F("Tag final recebida"));
          #endif
        // Termina a comunicação e sai da função imediatamente
        continue;
    }

    char startTagSRAM[startTagLen + 1];
    strcpy_P(startTagSRAM, startTag);

    char endTagSRAM[endTagLen + 1];
    strcpy_P(endTagSRAM, endTag);

    // Procura por <START> e <END> no buffer usando C-strings
    char* startPtr = strstr(incomingBuffer, startTagSRAM);
    char* endPtr = strstr(incomingBuffer, endTagSRAM);
    
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
        Serial.println(F("DEBUG: Rede ativa. Pulando validacao de PIN."));
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

    char permissionFileSRAM[permissionFileLen + 1];
    strcpy_P(permissionFileSRAM, permissionFile );
    
    // 2. Abre o arquivo para leitura
    permFile = SD.open(permissionFileSRAM, FILE_READ);
    
    if (!permFile) {
      
        #ifdef DEBUG_PIN_VALIDATION
        Serial.println(F("DEBUG: Erro ao abrir arquivo para leitura."));
        #endif
        return false;
    }
    
    char lineBuffer[32];
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
            Serial.print(F("DEBUG: Linha lida: "));
            Serial.println(lineBuffer);
            #endif

            // --- INICIO DA EXTRAÇÃO E VALIDAÇÃO ---
            

            // 1. EXTRAI E VALIDA O PIN
            char* storedPinStr = strtok(lineBuffer, ",");
            
            if (storedPinStr) {
                
                // Valida o PIN (1o campo)
                if (strcmp(storedPinStr, pin) == 0) {
                     
                    #ifdef DEBUG_PIN_VALIDATION
                    Serial.println(F("DEBUG: PIN Encontrado. Verificando Horário."));
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
                        short currentTotalMins = currentHour * 60L + currentMinute;
                        short startTotalMins = startHour * 60L + startMinute;
                        short endTotalMins = endHour * 60L + endMinute;

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
                            Serial.println(F("DEBUG: TEMPO E PIN SUCESSO! Acesso Concedido."));
                            #endif
                            break; // Sai do loop principal
                        } else {
                          
                            #ifdef DEBUG_PIN_VALIDATION
                            Serial.print(F("DEBUG: PIN OK, mas FORA DO TEMPO."));
                            #endif
                        }
                    } else {
                    
                        #ifdef DEBUG_PIN_VALIDATION
                        Serial.println(F("DEBUG: PIN OK, mas FORA DA DATA. Continuando a busca."));
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
    permFile = SD.open(permissionFileSRAM, FILE_WRITE);

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
     Serial.println(F("Falha ao Iniciar SD Card"));
    return;
  }

  Serial.println(F("Sucesso ao Iniciar SD Card"));
}



void callkeypadInput(){
  char key = keypad.getKey();

  if(key){
    handleKey(key);
    Serial.println(displayfeedback);
  }
}

void updateDisplay(char* msg){
  lcd.clear();
  lcd.setCursor(0, 0);


  // Verifica se a string (copiada na RAM) está vazia (strlen usa RAM)
  if (strlen(msg) == 0) {
    lcd.print("Pin: ");
  } else {
  lcd.print(msg);
  }
  
}

void handleKey(char key){
    if(key == '#'){
      updateDisplay("validando...");
      startValidation();
      clearFeedback();
    
      return;
    }

    if(key == '*'){
      removeLastKey();
      updateDisplay(displayfeedback);
      return;
    }

    if((feedbackPointer +1) == maxDisplayNum){
      
      return;
    }

    displayfeedback[feedbackPointer] = key;
    feedbackPointer++;
    updateDisplay(displayfeedback);
}



void removeLastKey(){
  feedbackPointer--;
  displayfeedback[feedbackPointer] = '\0';
}

void startValidation(){
 
  bool result = validatePin(displayfeedback);
  Serial.println(result);

  if(result){ 
    #ifdef LOCK_ON
    activateLock();
    #endif

    updateDisplay("Tranca Liberada!");
    delay(2000);
    updateDisplay("");
  } else {
    updateDisplay("Pin Invalido!");
    delay(2000);
    updateDisplay("");
  }
}

void clearFeedback(){
  feedbackPointer = 0;
  memset(displayfeedback, '\0', sizeof(displayfeedback));
}

void setup() {
  Serial.begin(9600);
  pinMode(lockPin, OUTPUT);
  digitalWrite(lockPin, LOW);
  Wire.begin();

  #ifdef SDCARD_ON
  startSDCard();
  #endif

  #ifdef LCD_ON
  startLCD();
  #endif


  #ifdef RTC_ON
  startRTC();
  #endif


  delay(1500);

  #ifdef TCP_ON
  startNetworkSetup();
  #endif
}

void loop() {
  #ifdef TCP_ON
    #ifdef SCHEDULEDMODE
      if (currentHour == TARGET_HOUR && !hasExecutedToday) {
          if (connectToServer()) { 
              handleEthernetCommunication();   
            }
          }
      #else
      handleEthernetCommunication();
      handleDisconnect(); 
      #endif
  #endif

  #ifdef KEYBOARD_ON
  callkeypadInput();
  #endif

  #ifdef RTC_ON
  updateCurrentTime();
  #endif
}