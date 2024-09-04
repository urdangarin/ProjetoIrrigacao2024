#include "String.h"
#include <HTTPClient.h>
#include <WiFi.h>

//variáveis de controle do sistema
const int sensorPin = 34; //entrada do sensor
const int valvPin = 32; //entradda do relé
const float DRY = 15; //limiar de umidade - seco
const float SOAKED = 30; //limiar de umidade - molhado
float moisture; //variável de umidade
boolean dripping = 0; //variável booleana para monitorar a irrigação
//int timeSinceLastSend = 0;

//variáveis de conexão
const char *ssid = "Enzo";
const char *pass = "16062003";
char *server = "script.google.com";
char *GScriptId = "1n14hYrKd4es7MwOKs5sh0VLfxHEyFF_8xGqw6Q6YCLY";
int httpport = 443;

//controle watchdog
hw_timer_t *timer = NULL;

//função que lê a umidade por determinada quantidade de segundos e faz a média aritmética dos valores encontrados
float getMoistureAvg(int seconds) {
  float sum = 0.0;
  float avg = 0.0;
  float result = 0.0;
  float valMap = 0.0;
  int ara0 = 0;
     
  for(int i = 0; i < seconds; i++) {
    timerWrite(timer, 0);
    ara0 = analogRead(sensorPin);  
    valMap = map(ara0, 4095, 0, 0 , 100);
    sum += valMap;
    delay(1000);//Espera um segundo
  } 
   
  result = sum/seconds;
  return result;
}

//função para conectar wifi
void connectWifi() {
  Serial.print("Connecting to: ");
  Serial.print(ssid);
  WiFi.begin(ssid, pass);
  int count = 0;
  while(WiFi.status() != WL_CONNECTED) { 
    Serial.print(".");
    delay(1000);

    if(count >= 60) {
      WiFi.disconnect();
      break;
    }

    timerWrite(timer, 0);
    count++;
  }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Conectado");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP()); 
  }
    
}

//função para enviar dados à planilha
void sendInfo(float moisture, boolean dripping, boolean stoppedDripping, boolean startedDripping) {
  HTTPClient http;
  //URL vem da planilha em si
  String url = String("https://script.google.com/macros/s/AKfycby0xDElrNz8tPjwY4GRxe68S1xOzq57zWz95mhiBh0Z13zWAcAw-vHp-ZuP7SyY_g/exec?") + "value1=" + moisture + "&value2=" + dripping + "&value3=" + stoppedDripping + "&value4=" + startedDripping;

  Serial.println("Sending data");
  http.begin(url.c_str());
  int httpCode = http.GET();
  //String payload;
  if (httpCode > 0) { 
    //payload = http.getString();
    Serial.print("httpCode: ");
    Serial.println(httpCode);
    //Serial.println(payload);
  }
  else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpCode);
  }
  http.end();
}

void setup() {
  //hw_timer_t * timerBegin(uint8_t num, uint16_t divider, bool countUp)
  
  //num: é a ordem do temporizador. Podemos ter quatro temporizadores, então a ordem pode ser [0,1,2,3].
  //divider: É um prescaler (reduz a frequencia por fator). Para fazer um agendador de um segundo, 
  //usaremos o divider como 80 (clock principal do ESP32 é 80MHz). Cada instante será T = 1/(80) = 1us
  //countUp: True o contador será progressivo
  
  //timer = timerBegin(0, 80, true); //timerID 0, div 80
  //timer, callback, interrupção de borda
  //timerAttachInterrupt(timer, &resetModule, true);
  //timer, tempo (us), repetição
  //timerAlarmWrite(timer, 10000000, true);
  //timerAlarmEnable(timer); //habilita a interrupção 
  
  Serial.begin(115200); //Open Serial connection for debugging   rive
  Serial.println("desliga o relé");
  pinMode(sensorPin, INPUT);
  pinMode(valvPin, OUTPUT);
  digitalWrite(valvPin, HIGH); 

  //apresentação
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  //timerWrite(timer, 0); //reseta o temporizador (alimenta o watchdog) 
  
  //confere a conexão wifi, se não estiver funcionando, tenta conectar
  if(WiFi.status() != WL_CONNECTED){
    connectWifi();
  }
  
  //lê a umidade por determinados segundos, 1 valor para cada segundo, (5 nesse caso) e retorna uma média da umidade nesse intervalo de temp
  moisture = getMoistureAvg(10);
  Serial.print("umidade em: ");
  Serial.println(moisture);
  Serial.print("Dripping: ");
  Serial.println(dripping);
  Serial.println("----------------------------------------");
  //sendInfo(moisture, dripping, 0, 0);
  
  //Serial.println(millis()/1000 - timeSinceLastSend);
  //if(millis()/1000 - timeSinceLastSend >= 60){
  //  
  //  timeSinceLastSend = millis()/1000;
  //}
  
  //se a umidade for menor que o limiar, liga o relé, que começa a regar
  if(moisture < DRY && not(dripping)) {
    //teste doggo
    //while(true) {
    //  Serial.println("travado, pae");
    //  delay(500);    
    //}
    
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(valvPin, LOW);//liga o relé
    Serial.print("motor ligado em umidade:");
    Serial.println(moisture);
    Serial.println("-------------IRRIGANDO---------------------------");
    dripping = 1;
    //if(WiFi.status() == WL_CONNECTED) {
    //  sendInfo(moisture, dripping, 0, 1);
    //}
    
  } else if (moisture >= SOAKED && dripping) {
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(valvPin, HIGH);//desliga o relé
      Serial.print("motor desligado em umidade: ");
      Serial.println(moisture);
      Serial.println("----------------PARAR DE IRRIGAR------------------------");
      dripping = 0;    
      //if(WiFi.status() == WL_CONNECTED) {
      //  sendInfo(moisture, dripping, 1, 0);
      //}
  }
  //delay(5000);
}
