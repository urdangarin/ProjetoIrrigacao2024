#include <esp_now.h>
#include <WiFi.h>

const int valvPin = 32; //entrada do relé

const float DRY = 15; //limiar de umidade - seco
const float SOAKED = 30; //limiar de umidade - molhado
bool dripping = false; //booleano do relé
const int MAX_BOARDS = 5;

float avg_moisture = 0;
int numBoards = 0;

//Estrutura de dados das placas masters
typedef struct struct_message {
  int id;
  float moisture;
} struct_message;

//Array com a estrtutra das placas conectadas
struct_message boardsStruct[MAX_BOARDS];

//Função callback quando recebe dados
void OnDataRecv(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  char macStr[18];
  Serial.print("Dados recebidos de: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           info->src_addr[0], info->src_addr[1], info->src_addr[2], 
           info->src_addr[3], info->src_addr[4], info->src_addr[5]);
  Serial.println(macStr);
  
  struct_message receivedData;
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  bool boardExists = false;

  //verifica se a placa já existe no array para atualizar o valor de umidade
  for(int i = 0; i < numBoards; i++){
    if(boardsStruct[i].id == receivedData.id){
      boardsStruct[i] = receivedData;
      boardExists = true;
      break;
    }
  }

  //ou adicioná-la no array caso não exista
  if(!boardExists && numBoards < MAX_BOARDS){
    boardsStruct[numBoards] = receivedData;
    numBoards++;
  }

  Serial.println();
}

//Calcula a média de umidade recebidas das placas
void calcAvgMoisture(){
  float totalMoisture = 0;

  if(numBoards == 0){
    avg_moisture = 0;
    return;
  }

  for(int i = 0; i < numBoards; i++){
    totalMoisture += boardsStruct[i].moisture;
  }

  avg_moisture = totalMoisture/numBoards;
}
 
void setup() {
  //Inicializa o monitor na frequência da ESP
  Serial.begin(115200);
  
  //Seta a ESP no modo Wi-fi Station
  WiFi.mode(WIFI_STA);

  //Inicia o ESP NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }
  
  // Register for recv CB to get received packet info
  esp_now_register_recv_cb(OnDataRecv);
  pinMode(valvPin, OUTPUT);
  digitalWrite(valvPin, HIGH); 

  //apresentação
  pinMode(LED_BUILTIN, OUTPUT);
}
 
void loop() {
  calcAvgMoisture();
  for(int i = 0; i < numBoards; i++){
    Serial.printf("Umidade da Placa ID %u: %.2f\n", boardsStruct[i].id, boardsStruct[i].moisture);
  }
  Serial.printf("Umidade Média: %.2f\n", avg_moisture);

  if(avg_moisture < DRY && not(dripping)){
    digitalWrite(LED_BUILTIN, LOW); //desliga luz azul na esp32
    digitalWrite(valvPin, LOW);//liga o relé
    Serial.printf("Relé ligado em umidade: %.2f\n", avg_moisture);
    Serial.println("-------------IRRIGANDO-------------");
    dripping = true;
  } else if (avg_moisture >= SOAKED && dripping) {
      digitalWrite(LED_BUILTIN, HIGH); //liga luz azul na esp32
      digitalWrite(valvPin, HIGH); //desliga o relé
      Serial.printf("Relé desligado em umidade: %.2f\n", avg_moisture);
      Serial.println("-------------PARAR DE IRRIGAR-------------");
      dripping = false;
  }

  Serial.println();
  delay(10000);  
}
