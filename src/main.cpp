/*
  ||        //  PROJETO DELP - CONECTION ESP  \\         ||

*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <TimeLib.h>

// ----------- REDE ------------

const char* SSID = "RoteadorVivoTCS";
const char* PASSWORD = "3133331999";

//const char* SSID = "DELP_IOT";
//const char* PASSWORD = "2023f4ff7872c2";

//const char* SSID = "TCS_2G";
//const char* PASSWORD = "33331999";

WiFiClient wifiClient;

// ---------- BUFFER -------------

#define BUFFER_SIZE 50
char buffer[BUFFER_SIZE][200];
int bufferIndex = 0;

// ---------- CONEXÃO (SERVIDOR - BROKER - NTP) -----------

unsigned long StartTime;

String mac;
const char* endpoint_host = "delp.tcsapp.com.br";
const uint16_t endpoint_port = 443;

PubSubClient MQTT(wifiClient);

//server ntp configuração

const char* ntpServer = "pool.ntp.org";
unsigned long epochTime; 

const char* BROKER_MQTT = "54.235.29.216";
const char* mqttUser = "Vn1zj0dwxiX9CmBM";
const char* mqttPassword = "ld39C62kLj0Jv9VIxsmdnm257i45pP6H";
const int BROKER_PORT = 1883;

bool conectBroker = false;
bool conectWifi = false;

#define ID_MQTT "BCI1"
#define TOPIC_PUBLISH "esp_client"

// Declaração das Funções
void mantemConexoes();
void conectaWiFi();
void conectaMQTT();
void enviaValores(float tensao, float corrente);


void fazerRequisicaoHTTP(const char* host, uint16_t port, const char* uri, const char* requestBody) {
  
  HTTPClient http;
  WiFiClientSecure client;

  client.setInsecure();

  const unsigned long connectionTimeout = 30000; // 30 segundos
  unsigned long startTime = millis();

  Serial.println("Iniciando requisicao HTTP...");

  if (WiFi.status() != WL_CONNECTED) {
       Serial.println("WiFi nao conectado!");
       return;
  }


  Serial.print("Endpoint: ");
  Serial.println(uri);
  Serial.print("Request body: ");
  Serial.println(requestBody);


  Serial.println("Tentando iniciar conexao HTTP...");
   while (!client.connect(host, port)) {
    if (millis() - startTime >= connectionTimeout) {
      Serial.println("Timeout de conexão atingido.");
      return;
    }
    Serial.println("Tentando conectar novamente em 1 segundo...");
    delay(1000);
  }

  Serial.println("Conexão estabelecida!");
  http.begin(client, host, port, uri);

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
  client.stop();
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

void getTimeComponents(unsigned long timestamp, int &hours, int &minutes, int &seconds) {
  seconds = timestamp % 60; // Obtém os segundos
  timestamp /= 60;
  minutes = timestamp % 60; // Obtém os minutos
  timestamp /= 60;
  hours = timestamp % 24;   // Obtém as horas
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

        enviaValores(tensao, corrente);
      // Enviar valores para o broker MQTT

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
        const char* endpoint = "/delp/arduino/status";
        String requestBody = "{\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint_host, endpoint_port, endpoint, requestBody.c_str());
      }
      
      if (acao == 2) // login
      {
        const char* endpoint = "/delp/arduino/login";
        String requestBody = "{\"matricula\":\"" + matricula + "\",\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint_host, endpoint_port, endpoint, requestBody.c_str());
      }

      if (acao == 3) // logout
      { 
        const char* endpoint = "/delp/arduino/logout";
        String requestBody = "{\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint_host, endpoint_port, endpoint, requestBody.c_str());
      }

      if (acao == 4) // INICIO PROCESSO MÁQUINA
      {
        const char* endpoint = "/delp/arduino/inicioProcesso";
        String requestBody = "{\"matricula\":\"" + matricula + "\",\"mac\":\"" + mac + "\",\"ordemProducao\":\"" + ordemProducao + "\",\"atividade\":\"" + atividade + "\",\"material\":\"" + material + "\"}";

        fazerRequisicaoHTTP(endpoint_host, endpoint_port, endpoint, requestBody.c_str());

        StartTime = getTime();

        int hours, minutes, seconds;
        getTimeComponents(StartTime, hours, minutes, seconds);

        String timeString = String(hours) + "-" + String(minutes) + "-" + String(seconds);
        Serial.print(timeString);
      }

      if (acao == 5)  //TERMINO PROCESSO MÁQUINA
      {
        const char* endpoint = "/delp/arduino/terminoProcesso";
        String requestBody = "{\"matricula\":\"" + matricula + "\",\"mac\":\"" + mac + "\",\"ordemProducao\":\"" + ordemProducao + "\",\"atividade\":\"" + atividade + "\",\"material\":\"" + material + "\"}";
        fazerRequisicaoHTTP(endpoint_host, endpoint_port, endpoint, requestBody.c_str());
        
      }

      if (acao == 6) //PAUSA PROCESSO
      { 
        const char* endpoint = "/delp/arduino/pausaProcesso";
        String requestBody = "{\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint_host, endpoint_port, endpoint, requestBody.c_str());
      }

      if (acao == 7) //REINICIA PROCESSO
      { 
        const char* endpoint = "/delp/arduino/reiniciaProcesso";
        String requestBody = "{\"mac\":\"" + mac + "\"}";
        fazerRequisicaoHTTP(endpoint_host, endpoint_port, endpoint, requestBody.c_str());
      }
      
    }
  }
  delay(20);
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
  mac = WiFi.macAddress();
  Serial.println("MEC: "+ mac);
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
/*
  Maquina 1: Corrente: 28 | Tensão: 29
  Maquina 2: Corrente: 31 | Tensão: 30
*/

void enviaValores(float tensao, float corrente) {
  
  unsigned long epochTime = getTime(); // 
  double epochTimeMs = epochTime + (millis() % 1000) / 1000.0; // Adicionando milissegundos ao timestamp

  char mqttMessageTensao[300];
  sprintf(mqttMessageTensao, "{\"id_variavel\": %d, \"valor\": %.2f, \"data_hora\": %.3f}", 29, tensao, epochTimeMs); // Usando %.3f para incluir milissegundos

  char mqttMessageCorrente[300];
  sprintf(mqttMessageCorrente, "{\"id_variavel\": %d, \"valor\": %.2f, \"data_hora\": %.3f}", 28, corrente, epochTimeMs); // Usando %.3f para incluir milissegundos

  //Serial.println(mqttMessageCorrente); | DEBUG DOS VALORES ENVIADOS PARA O SERVIDOR |  

  MQTT.publish(TOPIC_PUBLISH, mqttMessageTensao);
  MQTT.publish(TOPIC_PUBLISH, mqttMessageCorrente);

  delay(200);

}