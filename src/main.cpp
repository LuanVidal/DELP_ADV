/*
  ||        //  PROJETO DELP - CONECTION ESP  \\         ||

  ---------------------- PENDENCIAS -----------------------

  [ ] - Corrigir erro a verificação de SLL, estabelecida pelo protocolo HTTPS 

*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <TimeLib.h>

const char* SSID = "TCS_2G";
const char* PASSWORD = "33331999";

String mac = "D8:BF:C0:4:99:F4";

BearSSL::WiFiClientSecure wifiClient;
PubSubClient MQTT(wifiClient);

//server ntp configuração
const char* ntpServer = "pool.ntp.org";
unsigned long epochTime; 

const char* BROKER_MQTT = "ec2-34-224-25-118.compute-1.amazonaws.com";
const char* mqttUser = "Vn1zj0dwxiX9CmBM";
const char* mqttPassword = "ld39C62kLj0Jv9VIxsmdnm257i45pP6H";
const int BROKER_PORT = 1883;

bool conectBroker = false;
bool conectWifi = false;

#define ID_MQTT "BCI01"
#define TOPIC_PUBLISH "esp_client"

// Declaração das Funções
void mantemConexoes();
void conectaWiFi();
void conectaMQTT();
void enviaValores(float tensao, float corrente);

void fazerRequisicaoHTTP(const char* endpoint, const char* requestBody) {
  
  Serial.println("Iniciando requisicao HTTP...");

  if (WiFi.status() != WL_CONNECTED) {
       Serial.println("WiFi nao conectado!");
       return;
  }

  wifiClient.setInsecure();

  Serial.print("Endpoint: ");
  Serial.println(endpoint);
  Serial.print("Request body: ");
  Serial.println(requestBody);

  HTTPClient http;


  http.setTimeout(10000);

  Serial.println("Tentando iniciar conexao HTTP...");
  if (!http.begin(wifiClient, endpoint)) {
    Serial.println("Falha ao iniciar a conexao HTTP.");
    return;
  }

  http.addHeader("Content-Type", "application/json");

  Serial.println("Tentando fazer POST...");
  int httpResponseCode = http.POST(requestBody);

  Serial.print("Resposta HTTP recebida. Codigo: ");
  Serial.println(httpResponseCode);

  String response = http.getString();

  switch (httpResponseCode) {
    case 200:
      Serial.println("Requisicao bem sucedida:");
      Serial.println(response);
      break;

    case 401:
      Serial.println(response);
      break;

    case -11:
      Serial.println("Erro de timeout na requisicao HTTP.");
      break;

    default:
      Serial.print("Erro na requisicao HTTP. Codigo: ");
      Serial.println(httpResponseCode);
      Serial.println(response);
      break;
  }

  http.end();
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void setup() {
  Serial.begin(115200);
  conectaWiFi();
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  configTime(0, 0, ntpServer);
}

void loop() {
  mantemConexoes();
  MQTT.loop();

  if (Serial.available()) {
    String mensagem = Serial.readStringUntil('\n');
    if (mensagem.startsWith("Tensao:")) {
      int posicaoInicio = mensagem.indexOf(":") + 1;
      int posicaoFim = mensagem.indexOf("V");
      String tensaoStr = mensagem.substring(posicaoInicio, posicaoFim);
      float tensao = tensaoStr.toFloat();

      posicaoInicio = mensagem.indexOf("Corrente:") + 9;
      posicaoFim = mensagem.indexOf("A");
      String correnteStr = mensagem.substring(posicaoInicio, posicaoFim);
      float corrente = correnteStr.toFloat();

      // Enviar valores para o broker MQTT
      enviaValores(tensao, corrente);
    }

    if (mensagem.startsWith("Acao:")) {
      int posicaoInicio = mensagem.indexOf(":") + 2;
      int posicaoFim = mensagem.indexOf(",");
      String acaoStr = mensagem.substring(posicaoInicio, posicaoFim);
      int acao = acaoStr.toInt();

      Serial.println(mensagem);  
      posicaoInicio = mensagem.indexOf("\"matricula\": \"") + 14;
      posicaoFim = mensagem.indexOf("\"", posicaoInicio);
      String matricula = mensagem.substring(posicaoInicio, posicaoFim);

      posicaoInicio = mensagem.indexOf("\"mac\": \"") + 8;
      posicaoFim = mensagem.indexOf("\"", posicaoInicio);
      

      posicaoInicio = mensagem.indexOf("\"ordemProducao\":") + 16;
      posicaoFim = mensagem.indexOf(",", posicaoInicio);
      int ordemProducao = mensagem.substring(posicaoInicio, posicaoFim).toInt();

      posicaoInicio = mensagem.indexOf("\"atividade\":") + 12;
      posicaoFim = mensagem.indexOf(",", posicaoInicio);
      int atividade = mensagem.substring(posicaoInicio, posicaoFim).toInt();

      posicaoInicio = mensagem.indexOf("\"material\":") + 11;
      posicaoFim = mensagem.indexOf("}", posicaoInicio);
      int material = mensagem.substring(posicaoInicio, posicaoFim).toInt();

      if (acao == 1) // status
      {
        const char* endpoint = "https://delp.tcsapp.com.br:443/delp/arduino/status";
        String requestBody = "{\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint, requestBody.c_str());
      }
      
      if (acao == 2) // login
      {
        const char* endpoint = "https://delp.tcsapp.com.br/delp/arduino/login";
        String requestBody = "{\"matricula\":\"" + matricula + "\",\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint, requestBody.c_str());
      }

      if (acao == 3) // logout
      { 
        const char* endpoint = "https://delp.tcsapp.com.br/delp/arduino/logout";
        String requestBody = "{\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint, requestBody.c_str());
      }

      if (acao == 4) // INICIO PROCESSO MÁQUINA
      {
        const char* endpoint = "https://delp.tcsapp.com.br/delp/arduino/inicioProcesso";
        String requestBody = "{\"matricula\":\"" + matricula + "\",\"mac\":\"" + mac + "\",\"ordemProducao\":" + String(ordemProducao) + ",\"atividade\":" + String(atividade) + ",\"material\":" + String(material) + "}";
        fazerRequisicaoHTTP(endpoint, requestBody.c_str());
      }

      if (acao == 5)  //TERMINO PROCESSO MÁQUINA
      {
        const char* endpoint = "https://delp.tcsapp.com.br/delp/arduino/terminoProcesso";
        String requestBody = "{\"matricula\":\"" + matricula + "\",\"mac\":\"" + mac + "\",\"ordemProducao\":" + String(ordemProducao) + ",\"atividade\":" + String(atividade) + ",\"material\":" + String(material) + "}";
        fazerRequisicaoHTTP(endpoint, requestBody.c_str());
      }

      if (acao == 6) //BUSCA ORDENS MÁQUINA
      { 
        const char* endpoint = "https://delp.tcsapp.com.br/delp/arduino/buscaOrdens";
        String requestBody = "{\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint, requestBody.c_str());
      }

      if (acao == 7) //BUSCA busca Atividades
      { 
        const char* endpoint = "https://delp.tcsapp.com.br/delp/arduino/buscaAtividades";
        String requestBody = "{\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint, requestBody.c_str());
      }
      
      if (acao == 8) //Busca Materiais
      { 
        const char* endpoint = "https://delp.tcsapp.com.br/delp/arduino/buscaMateriais";
        String requestBody = "{\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint, requestBody.c_str());
      }
    }
  }
}
 
void mantemConexoes() {
  if (!MQTT.connected()) {
    conectaMQTT();
  }
  conectaWiFi();
}

void conectaWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  Serial.print("Conectando-se na rede: ");
  Serial.print(SSID);
  Serial.println("  Aguarde!");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Conectado com sucesso, na rede: ");
  Serial.print(SSID);
  Serial.println("IP obtido: ");
  Serial.print(WiFi.localIP());
  String mec = WiFi.macAddress();
}

void conectaMQTT() {
  while (!MQTT.connected()) {
    Serial.print("Conectando ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("\nConectado ao Broker com sucesso!");
    } else {
      Serial.println("Nao foi possivel se conectar ao broker.");
      Serial.println("Nova tentativa de conexao em 5s");
      delay(5000);
    }
  }
}

void enviaValores(float tensao, float corrente) {
  unsigned long epochTime = getTime(); // 
  double epochTimeMs = epochTime + (millis() % 1000) / 1000.0; // Adicionando milissegundos ao timestamp

  char mqttMessageTensao[200];
  sprintf(mqttMessageTensao, "{\"id_variavel\": %d, \"valor\": %.2f, \"data_hora\": %.3f}", 30, tensao, epochTimeMs); // Usando %.3f para incluir milissegundos

  char mqttMessageCorrente[200];
  sprintf(mqttMessageCorrente, "{\"id_variavel\": %d, \"valor\": %.2f, \"data_hora\": %.3f}", 31, corrente, epochTimeMs); // Usando %.3f para incluir milissegundos

  MQTT.publish(TOPIC_PUBLISH, mqttMessageTensao);
  MQTT.publish(TOPIC_PUBLISH, mqttMessageCorrente);
  delay(200);
}