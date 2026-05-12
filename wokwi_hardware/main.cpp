#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Definições de Pinos
#define DHTPIN 15
#define DHTTYPE DHT22
#define POT_BPM_PIN 34
#define BTN_WIFI_PIN 12

DHT dht(DHTPIN, DHTTYPE);

// Configurações de Wi-Fi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "broker.hivemq.com"; 

const char* topico_publicacao = "cardioia/nicolas/sinais"; 

WiFiClient espClient;
PubSubClient client(espClient);

// Variáveis de controle de conexão simulada pelo botão
bool wifiConectadoSimulado = true;
bool ultimoEstadoBotao = HIGH;

// Estrutura de dados que simula o arquivo do SPIFFS (Edge Computing)
struct DadosCardio {
  float temperatura;
  int bpm;
};

const int TAMANHO_BUFFER = 20;
DadosCardio bufferOffline[TAMANHO_BUFFER];
int contagemBuffer = 0;

unsigned long ultimoTempoLeitura = 0;
const long intervaloLeitura = 3000;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando na rede: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[REDE] Wi-Fi Real Conectado!");
}

void reconnect_mqtt() {
  // Loop até conectar ao broker MQTT
  while (!client.connected() && wifiConectadoSimulado) {
    Serial.print("Tentando conexao MQTT...");
  
    String clientId = "CardioIA_ESP32_";
    clientId += String(random(0, 0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado ao HiveMQ!");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(BTN_WIFI_PIN, INPUT_PULLUP); 
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  // Mantém a conexão MQTT viva se o sistema estiver online
  if (wifiConectadoSimulado) {
    if (!client.connected()) {
      reconnect_mqtt();
    }
    client.loop();
  }

  // 1. VERIFICAÇÃO DO BOTÃO (simula queda de rede)
  bool estadoBotao = digitalRead(BTN_WIFI_PIN);
  if (estadoBotao == LOW && ultimoEstadoBotao == HIGH) {
    wifiConectadoSimulado = !wifiConectadoSimulado;
    Serial.println("\n-----------------------------------------");
    if(wifiConectadoSimulado) {
      Serial.println("[STATUS] Rede Restaurada pelo Botao!");
    } else {
      Serial.println("[STATUS] Rede Derrubada pelo Botao! Modo Offline ativado.");
    }
    Serial.println("-----------------------------------------\n");
    delay(200);
  }
  ultimoEstadoBotao = estadoBotao;

  // 2. CAPTURA DE SINAIS VITAIS
  unsigned long tempoAtual = millis();
  if (tempoAtual - ultimoTempoLeitura >= intervaloLeitura) {
    ultimoTempoLeitura = tempoAtual;

    float temp = dht.readTemperature();
    int leituraPot = analogRead(POT_BPM_PIN);
    int bpm = map(leituraPot, 0, 4095, 40, 180);

    if (isnan(temp)) {
      Serial.println("[ERRO] Falha ao ler o DHT22!");
      return;
    }

    // JSON com os dados
    String payload = "{\"temperatura\": " + String(temp) + ", \"bpm\": " + String(bpm) + "}";

    // 3. LÓGICA DE EDGE COMPUTING E ENVIO
    if (wifiConectadoSimulado) {
      
      // Sincroniza dados acumulados offline
      if (contagemBuffer > 0) {
        Serial.println("[NUVEM] Sincronizando historico armazenado offline...");
        for (int i = 0; i < contagemBuffer; i++) {
          String histPayload = "{\"temperatura\": " + String(bufferOffline[i].temperatura) + ", \"bpm\": " + String(bufferOffline[i].bpm) + ", \"historico\": true}";
          client.publish(topico_publicacao, histPayload.c_str());
          Serial.println("   -> Historico enviado: " + histPayload);
          delay(500); // Latência simulada
        }
        contagemBuffer = 0;
        Serial.println("[NUVEM] Sincronizacao concluida.");
      }
      
      // Envia leitura em tempo real
      client.publish(topico_publicacao, payload.c_str());
      Serial.println("[MQTT] Publicado: " + payload);
      
    } else {
      // Salva localmente se estiver sem internet
      if (contagemBuffer < TAMANHO_BUFFER) {
        bufferOffline[contagemBuffer].temperatura = temp;
        bufferOffline[contagemBuffer].bpm = bpm;
        contagemBuffer++;
        
        Serial.print("[LOCAL] Salvo na memoria interna (");
        Serial.print(contagemBuffer);
        Serial.print("/");
        Serial.print(TAMANHO_BUFFER);
        Serial.println(") -> " + payload);
      }
    }
  }
}