#include "String.h"
#include <esp_now.h>
#include <WiFi.h>

//variáveis de controle do sistema
const int sensorPin = 34; //entrada do sensor
hw_timer_t *timer = NULL;

uint8_t broadcastAddress[] = {0xB4, 0x8A, 0x0A, 0x75, 0xA6, 0x58}; //Mac Addres da placa central

typedef struct struct_message {
    int id; //Deve ser único para cada placa
    float moisture;
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;
//função que lê a umidade por determinada quantidade de segundos e faz a média aritmética dos valores encontrados
float getMoistureAvg(int seconds) {
  float sum = 0.0;
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

void setup() {
  Serial.begin(115200); //Inicia o monitor na frequência da ESP

  WiFi.mode(WIFI_STA); //Entra no modo Wi-fi Station

  // Inicia o ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar o ESP-NOW");
    return;
  }

  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  myData.id = 1;
  myData.moisture = getMoistureAvg(10);

  Serial.print("Umidade: ");
  Serial.println(myData.moisture);
  Serial.println("=========================");

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println("Sucesso ao enviar");
  } 
  else {
    Serial.println("Erro ao enviar");
  }
  

  delay(1000);
}
