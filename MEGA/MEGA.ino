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
String acaoString = "1";

void exibirTela(Tela tela) {
  // Implemente o código para exibir a tela no display ou interface gráfica
  // Aqui, você deve exibir o conteúdo da tela com base no valor da variável "tela"
}

void enviaValores() {
    String mensagem = "Acao: " + acaoString + ", \"matricula\": \"" + String(matricula) + "\", \"mac\": \"123-456\", \"ordemProducao\": " + String(ordemProducao) + ", \"atividade\": " + String(atividade) + ", \"material\": " + String(material);
    Serial3.println(mensagem);
  }

void processarRespostaHTTP(const String& resposta) {
  // Parse da resposta JSON
  DynamicJsonDocument doc(1024); // Tamanho do buffer de 1024 bytes, ajuste conforme suas necessidades
  DeserializationError error = deserializeJson(doc, resposta);
  
  if (error) {
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
}

void loop() {

  // useabilidade ------------------------------------------------------------------

  switch (telaAtual) {
    case TELA_INICIAL:
      // Exibir tela inicial
      exibirTela(TELA_INICIAL);
      // Aguardar até que alguma tecla seja pressionada
      char tecla = keypad.getKey();
      if (tecla == '#') {
        // Avança para a tela de matrícula
        telaAtual = TELA_MATRICULA;
        matricula = 0; // Zera o valor da matrícula
      }
      break;

    case TELA_MATRICULA:
      // Exibir tela de matrícula
      exibirTela(TELA_MATRICULA);
      // Ler a entrada do teclado
      tecla = keypad.getKey();
      if (tecla >= '0' && tecla <= '9') {
        // Tecla numérica pressionada: adiciona o dígito à matrícula
        matricula = matricula * 10 + (tecla - '0');
      } else if (tecla == '#') {
        // Tecla "#" pressionada: avança para a tela de ordem de produção
        telaAtual = TELA_ORDEM_PRODUCAO;
        ordemProducao = 0; // Zera o valor da ordem de produção
      } else if (tecla == 'D') {
        // Tecla "D" pressionada: corrige a matrícula
        matricula = 0; // Zera o valor da matrícula
      }
      break;

    case TELA_ORDEM_PRODUCAO:
      // Exibir tela de ordem de produção
      exibirTela(TELA_ORDEM_PRODUCAO);
      // Ler a entrada do teclado
      tecla = keypad.getKey();
      if (tecla >= '0' && tecla <= '9') {
        // Tecla numérica pressionada: adiciona o dígito à ordem de produção
        ordemProducao = ordemProducao * 10 + (tecla - '0');
      } else if (tecla == '#') {
        // Tecla "#" pressionada: avança para a tela de atividade
        telaAtual = TELA_ATIVIDADE;
        atividade = 0; // Zera o valor da atividade
      } else if (tecla == 'D') {
        // Tecla "D" pressionada: corrige a ordem de produção
        ordemProducao = 0; // Zera o valor da ordem de produção
      } else if (tecla == '*') {
        // Tecla "*" pressionada: volta para a tela anterior
        telaAtual = TELA_MATRICULA;
        matricula = 0; // Zera o valor da matrícula
      }
      break;

    case TELA_ATIVIDADE:
      // Exibir tela de atividade
      exibirTela(TELA_ATIVIDADE);
      // Ler a entrada do teclado
      tecla = keypad.getKey();
      if (tecla >= '0' && tecla <= '9') {
        // Tecla numérica pressionada: adiciona o dígito à atividade
        atividade = atividade * 10 + (tecla - '0');
      } else if (tecla == '#') {
        // Tecla "#" pressionada: avança para a tela de material
        telaAtual = TELA_MATERIAL;
        material = 0; // Zera o valor do material
      } else if (tecla == 'D') {
        // Tecla "D" pressionada: corrige a atividade
        atividade = 0; // Zera o valor da atividade
      } else if (tecla == '*') {
        // Tecla "*" pressionada: volta para a tela anterior
        telaAtual = TELA_ORDEM_PRODUCAO;
        ordemProducao = 0; // Zera o valor da ordem de produção
      }
      break;

    case TELA_MATERIAL:
      // Exibir tela de material
      exibirTela(TELA_MATERIAL);
      // Ler a entrada do teclado
      tecla = keypad.getKey();
      if (tecla >= '0' && tecla <= '9') {
        // Tecla numérica pressionada: adiciona o dígito ao material
        material = material * 10 + (tecla - '0');
      } else if (tecla == '#') {
        // Tecla "#" pressionada: envia os valores e avança para a tela de aguarde
        enviaValores();
        telaAtual = TELA_AGUARDE;
      } else if (tecla == 'D') {
        // Tecla "D" pressionada: corrige o material
        material = 0; // Zera o valor do material
      } else if (tecla == '*') {
        // Tecla "*" pressionada: volta para a tela anterior
        telaAtual = TELA_ATIVIDADE;
        atividade = 0; // Zera o valor da atividade
      }
      break;

    case TELA_AGUARDE:
      // Exibir tela de aguarde
      exibirTela(TELA_AGUARDE);
      // Aguardar o retorno da mensagem do servidor para avançar para a tela RASTREABILIDADE1
      // Implemente aqui a lógica para aguardar o retorno da mensagem do servidor
      String respostaDoServidor = "Inicio de Processo Realizado Sem Erros";
      if (respostaDoServidor == "Inicio de Processo Realizado Sem Erros") {
        telaAtual = TELA_RASTREABILIDADE1;
      }
      break;

    case TELA_RASTREABILIDADE1:
      // Exibir tela de rastreabilidade 1
      exibirTela(TELA_RASTREABILIDADE1);
      // Ler a entrada do teclado
      tecla = keypad.getKey();
      if (tecla == '#') {
        // Tecla "#" pressionada: pausa o processo e avança para a tela RASTREABILIDADE2
        acao = 2; // Ação para pausar o processo
        enviaValores();
        telaAtual = TELA_RASTREABILIDADE2;
      } else if (tecla == '*') {
        // Tecla "*" pressionada: finaliza o processo e avança para a tela FINALIZAR_PROCESSO
        acao = 3; // Ação para finalizar o processo
        enviaValores();
        telaAtual = TELA_FINALIZAR_PROCESSO;
      }
      break;

    case TELA_RASTREABILIDADE2:
      // Exibir tela de rastreabilidade 2
      exibirTela(TELA_RASTREABILIDADE2);
      // Ler a entrada do teclado
      tecla = keypad.getKey();
      if (tecla == '#') {
        // Tecla "#" pressionada: volta para a tela RASTREABILIDADE1
        telaAtual = TELA_RASTREABILIDADE1;
      } else if (tecla == '*') {
        // Tecla "*" pressionada: avança para a tela FINALIZAR_PROCESSO
        telaAtual = TELA_FINALIZAR_PROCESSO;
      }
      break;

    case TELA_FINALIZAR_PROCESSO:
      // Exibir tela de finalizar processo
      exibirTela(TELA_FINALIZAR_PROCESSO);
      // Ler a entrada do teclado
      tecla = keypad.getKey();
      if (tecla == '#') {
        // Tecla "#" pressionada: confirma o finalizar processo e avança para a tela CONC
        acao = 5; // Ação para confirmar o finalizar processo
        enviaValores();
        telaAtual = TELA_CONC;
        delay(5000); // Aguarda 5 segundos antes de avançar para a tela MATRICULA
        telaAtual = TELA_MATRICULA;
      } else if (tecla == '*') {
        // Tecla "*" pressionada: cancela o finalizar processo e volta para a tela RASTREABILIDADE1
        telaAtual = TELA_RASTREABILIDADE1;
      }
      break;

    case TELA_CONC:
      // Exibir tela de conc
      exibirTela(TELA_CONC);
      // ... (implemente a lógica para a tela de conc)
      // Pode ser necessário um contador para atrasar o avanço para a tela MATRICULA após o delay de 5 segundos
      // Certifique-se de definir a variável "acao" corretamente para a próxima ação na tela MATRICULA
      break;
  }
  // recebimento de dados -----------------------------------------------------------

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

  // envio de dados ----------------------------------------------------------------------

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