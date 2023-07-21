#include <Keypad.h>
#include <ArduinoJson.h>

// Definição de variáveis globais
const byte LINHAS = 4; // Número de linhas do teclado matricial
const byte COLUNAS = 4; // Número de colunas do teclado matricial

const int MAX_RESPOSTA_SIZE = 1024; // Tamanho máximo da resposta (ajuste conforme suas necessidades)
char respostaBuffer[MAX_RESPOSTA_SIZE];
int respostaIndex = 0;
bool recebendoResposta = false;

char teclas[LINHAS][COLUNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pinosLinhas[LINHAS] = {9, 8, 7, 6}; // Pinos conectados às linhas do teclado
byte pinosColunas[COLUNAS] = {5, 4, 3, 2}; // Pinos conectados às colunas do teclado

Keypad keypad = Keypad(makeKeymap(teclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

int matricula = 12345;
int ordemProducao = 1;
int atividade = 1;
int material = 1;
int acao = 1;

enum Tela {
  TELA_INICIAL,
  TELA_MATRICULA,
  ERRO_TELA_MATRICULA,
  TELA_ORDEM_PRODUCAO,
  ERRO_TELA_ORDEM_PRODUCAO,
  TELA_ATIVIDADE,
  ERRO_TELA_ATIVIDADE,
  TELA_MATERIAL,
  ERRO_TELA_MATERIAL,
  TELA_AGUARDE,
  TELA_RASTREABILIDADE1,
  TELA_RASTREABILIDADE2,
  TELA_RASTREABILIDADE3,
  TELA_FINALIZAR_PROCESSO,
  TELA_CONC
};

Tela telaAtual = TELA_INICIAL;

void processarRespostaHTTP(const String& resposta) {
  // Parse da resposta JSON
  DynamicJsonDocument doc(1024); // Tamanho do buffer de 1024 bytes, ajuste conforme suas necessidades
  DeserializationError error = deserializeJson(doc, resposta);
  
  if (error) {
    Serial.println("Erro ao processar a resposta JSON");
    return;
  }

  // Verificar o tipo de resposta e tomar as ações correspondentes
  if (doc.containsKey("id") && doc.containsKey("mac") && doc.containsKey("nome")) {
    // RESPOSTA 1: Informações da Máquina
    int id = doc["id"];
    String mac = doc["mac"].as<String>();
    String nome = doc["nome"].as<String>();
  
  }
  else if (doc.containsKey("ordens")) {
    // RESPOSTA 2: Lista de Ordens de Produção
    int count = doc["ordens"]["count"];

    JsonArray rows = doc["ordens"]["rows"];
    for (JsonObject row : rows) {
      int ordemId = row["id"];
      String codigo = row["codigo"].as<String>();

    }
  }
  else if (doc.containsKey("message")) {
    // RESPOSTA 3, 4, 5, 6, 7 ou 8: Mensagem de operação realizada com sucesso
    String message = doc["message"].as<String>();
    // Faça o que for necessário com a mensagem
  }
  else {
    Serial.println("Resposta desconhecida ou inválida.");
  }
}

void setup() {
  Serial.begin(115200);
  Serial3.begin(115200);
  String mensagem = "Ação: 1, \"matricula\": \"" + String(matricula) + "\", \"mac\": \"123-456\", \"ordemProducao\": " + String(ordemProducao) + ", \"atividade\": " + String(atividade) + ", \"material\": " + String(material);
  Serial3.println(mensagem);
}

void loop() {

  while (Serial3.available() && !recebendoResposta) {
    char data = Serial3.read();
    
    if (data == '{') {
      recebendoResposta = true;
      respostaIndex = 0;
    }
  }

  if (recebendoResposta) {
    // Armazena os dados recebidos até atingir o tamanho máximo da resposta
    if (respostaIndex < MAX_RESPOSTA_SIZE - 1) {
      char data = Serial3.read();
      if (data == '}') {
        recebendoResposta = false;
        respostaBuffer[respostaIndex] = '\0'; // Adiciona o caractere nulo para formar a string
        String resposta = String(respostaBuffer);
        // Processar a resposta recebida
        processarRespostaHTTP(resposta);
      } else {
        respostaBuffer[respostaIndex] = data;
        respostaIndex++;
      }
    } else {
      // Tamanho máximo da resposta atingido, encerra a leitura
      recebendoResposta = false;
      respostaIndex = 0;
      Serial.println("Tamanho máximo da resposta excedido. Descartando resposta.");
    }
  }

  if (Serial3.available()) {
    //Leitura de um byte.
    char data = Serial3.read();
    //Imprima o mesmo dado pela porta usb.
    Serial.print(data);
    //Acrescente o caractere recebido a string de mensagem.
    delay(10);
  }

  float tensao = analogRead(A0);
  float corrente = analogRead(A1);

  float tensaoMapeada =  tensao * 0.0488758553274682; // 0 a 50 - 0.0488758553274682
  float correnteMapeada = corrente * 0.2932551319648094; // 0 a 300 - 0.2932551319648094

  // Envia os dados para o ESP8266 via comunicação serial
  Serial3.print("Tensao:");
  Serial3.print(tensaoMapeada);
  Serial3.print("V | Corrente:");
  Serial3.print(correnteMapeada);
  Serial3.println("A");

  delay(500); // Intervalo de envio dos dados para o ESP8266
}