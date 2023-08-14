#include <Keypad.h>
#include <ArduinoJson.h>

// Definição de variáveis globais
String mensagemDeErroRecebida;
float correnteMapeada = 0.0;
float tensaoMapeada = 0.0;

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

long int matricula = 0;
int ordemProducao;
int atividade;
int material;
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

const int MAX_DIGITOS_MATRICULA = 4;
const int MAX_DIGITOS_ORDEM_PRODUCAO = 4;
const int MAX_DIGITOS_ATIVIDADE = 1;
const int MAX_DIGITOS_MATERIAL = 1;


// definicão dos elementos da tela
#define MATRICULA           0x20
#define ORDEM_PRODUCAO      0x21
#define ATIVIDADE           0x22
#define MATERIAL            0x23

// definição tela rastreabilidade (1)
#define MATRICULA1          0x24
#define ORDEM_PRODUCAO1      0x25
#define ATIVIDADE1           0x26
#define MATERIAL1            0x27

#define TENSAO1              0x28
#define CORRENTE1            0x29
#define INICIO1              0x30
#define TEMPO_PERCORRIDO1    0x31

// definição tela rastreabilidade (2)
#define MATRICULA2           0x32
#define ORDEM_PRODUCAO2      0x33
#define ATIVIDADE2           0x34
#define MATERIAL2            0x35

#define TENSAO2              0x36
#define CORRENTE2            0x37
#define INICIO2              0x38
#define TEMPO_PERCORRIDO2    0x39

// definição tela rastreabilidade (3)
#define MATRICULA3           0x40
#define ORDEM_PRODUCAO3      0x41
#define ATIVIDADE3           0x42
#define MATERIAL3            0x43

#define TENSAO3              0x44
#define CORRENTE3            0x45
#define INICIO3              0x46
#define TEMPO_PERCORRIDO3    0x47


Tela telaAtual = TELA_INICIAL;
String acaoString = "1";

// AÇÕES

void beep(){
  unsigned char DataBeep[8] = {0x5A, 0xA5, 0x05, 0x82, 0x00, 0xA0, 0x00, 0x0A};
  Serial1.write(DataBeep, 9);
}

void escreveTela(int endereco, int valor){
  unsigned char DataText[8] = {0x5A, 0xA5, 0x05, 0x82, endereco, 0x00, 0x00, 0x00}; // no 0x20 mudar para vp do elemento a ser mudado (enviar dados)
  DataText[6] = highByte(valor);
  DataText[7] = lowByte(valor);

  Serial1.write(DataText, 9);
  delay(10);
}

void exibirTela(int tela) {
  unsigned char Data[10] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x84, 0x5a, 0x01, 0x00, tela};
  Serial1.write(Data, 10);
}

void enviaValores() {
    String mensagem = "Acao: " + acaoString + ", \"matricula\": \"" + String(matricula) + "\", \"mac\": \"123-456\", \"ordemProducao\": " + String(ordemProducao) + ", \"atividade\": " + String(atividade) + ", \"material\": " + String(material);
    
    Serial3.println(mensagem); // Envie a mensagem
    
    // Aguarde a confirmação de envio
    while (!Serial3.available()) {
        // Espera até que algum dado esteja disponível na porta Serial3
    }
    
    // Agora você pode considerar que a mensagem foi enviada com sucesso
    // e continuar com o resto do código
}


void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial3.begin(115200);
}

void loop() {

  // useabilidade ------------------------------------------------------------------

  char tecla = keypad.getKey();

  switch (telaAtual) {
    case TELA_INICIAL:
      // Exibir tela inicial
        exibirTela(0);

      // Aguardar até que alguma tecla seja pressionada
      if (tecla == '#') {
        // Avança para a tela de matrícula

        beep();

        acaoString = "1" ;//status
        enviaValores();

        telaAtual = TELA_MATRICULA;

        escreveTela(MATRICULA, 00000); // Zera o valor da matrícula
        exibirTela(1);
      }
    break;

    case TELA_MATRICULA:
      
      
      // Exibir tela de matrícula
      exibirTela(1);
      // Ler a entrada do teclado
      if (tecla >= '0' && tecla <= '9' && matricula < pow(10, MAX_DIGITOS_MATRICULA)) {
        // Tecla numérica pressionada: adiciona o dígito à matrícula
        matricula = matricula * 10 + (tecla - '0');
        escreveTela(MATRICULA, matricula);

      }  else if (tecla == '#') {
        // Tecla "#" pressionada: avança para a tela de ordem de produção

        beep();

        acaoString = "2";
        enviaValores(); // informar a matricula digitada para servidor (FAZ O LOGIN DO OPERADOR)7
        
        exibirTela(3);
        
        escreveTela(ORDEM_PRODUCAO, 00000);
        telaAtual = TELA_ORDEM_PRODUCAO;
        

      } else if (tecla == 'D') {
        // Tecla "D" pressionada: corrige a matrícula
        matricula = 00000;
        escreveTela(MATRICULA, 00000);
      }
      break;

    case TELA_ORDEM_PRODUCAO:
      // Exibir tela de ordem de produção
      exibirTela(3);


      // Ler a entrada do teclado
      if (tecla >= '0' && tecla <= '9' && ordemProducao < pow(10, MAX_DIGITOS_ORDEM_PRODUCAO)) {
        // Tecla numérica pressionada: adiciona o dígito à ordem de produção
        ordemProducao = ordemProducao * 10 + (tecla - '0');
        escreveTela(ORDEM_PRODUCAO, ordemProducao);

      } else if (tecla == '#') {
        // Tecla "#" pressionada: avança para a tela de atividade
        telaAtual = TELA_ATIVIDADE;
        escreveTela(ATIVIDADE, 00);

        beep();
        exibirTela(5);

      } else if (tecla == 'D') {
        // Tecla "D" pressionada: corrige a ordem de produção
        ordemProducao = 00000; // Zera o valor da ordem de produção
        escreveTela(ORDEM_PRODUCAO, 00000);

      } else if (tecla == '*') {
        // Tecla "*" pressionada: volta para a tela anterior
        telaAtual = TELA_MATRICULA;
        matricula = 00000; // Zera o valor da matrícula
        escreveTela(MATRICULA, 00000);
        exibirTela(1);
      }
      break;

    case TELA_ATIVIDADE:
      // Exibir tela de atividade
      exibirTela(5);
      // Ler a entrada do teclado
      if (tecla >= '0' && tecla <= '9' && atividade < pow(10, MAX_DIGITOS_ATIVIDADE)) {
        // Tecla numérica pressionada: adiciona o dígito à atividade
        atividade = atividade * 10 + (tecla - '0');
        escreveTela(ATIVIDADE, atividade);

      } else if (tecla == '#') {
        // Tecla "#" pressionada: avança para a tela de material
        telaAtual = TELA_MATERIAL;
        material = 00; // Zera o valor do material
        escreveTela(MATERIAL, 00);
        
        beep();
        exibirTela(7);

      } else if (tecla == 'D') {
        // Tecla "D" pressionada: corrige a atividade
        atividade = 00; // Zera o valor da atividade
        escreveTela(ATIVIDADE, 00);

      } else if (tecla == '*') {
        // Tecla "*" pressionada: volta para a tela anterior
        telaAtual = TELA_ORDEM_PRODUCAO;
        ordemProducao = 00000; // Zera o valor da ordem de produção
        escreveTela(ORDEM_PRODUCAO, 00000);
        exibirTela(3);
      }
      break;

    case TELA_MATERIAL:
      // Exibir tela de material
      exibirTela(7);
      // Ler a entrada do teclado
      if (tecla >= '0' && tecla <= '9' && material < pow(10, MAX_DIGITOS_MATERIAL)) {
        // Tecla numérica pressionada: adiciona o dígito ao material
        material = material * 10 + (tecla - '0');
        escreveTela(MATERIAL, material);

      } else if (tecla == '#') {
        // Tecla "#" pressionada: envia os valores e avança para a tela de aguarde

        telaAtual = TELA_AGUARDE;

        beep();
        exibirTela(9);

      } else if (tecla == 'D') {
        // Tecla "D" pressionada: corrige o material
        material = 00; // Zera o valor do material
        escreveTela(ATIVIDADE, 00);
      } else if (tecla == '*') {
        // Tecla "*" pressionada: volta para a tela anterior
        telaAtual = TELA_ATIVIDADE;
        atividade = 00; // Zera o valor da atividade
        escreveTela(ATIVIDADE, 00000);
        exibirTela(3);
      }
      break;

    case TELA_AGUARDE:
      // Exibir tela de aguarde
      exibirTela(9);

      //inicar trabalho
      acaoString = "4";
      enviaValores();
      

      if (mensagemDeErroRecebida == "Operador Não Encontrado!") {
        telaAtual = ERRO_TELA_MATRICULA;
      }

      if (mensagemDeErroRecebida == "Atividade Não Encontrada!"){
        telaAtual = ERRO_TELA_ATIVIDADE;
      }

      if (mensagemDeErroRecebida == "Ordem de Produção Não Encontrada!"){
        telaAtual = ERRO_TELA_ORDEM_PRODUCAO;
      }

      if (mensagemDeErroRecebida == "Material Não Encontrado!"){
        telaAtual = ERRO_TELA_MATERIAL;
      }

      if(mensagemDeErroRecebida == ""){
        telaAtual = TELA_RASTREABILIDADE1;

        beep();
        exibirTela(10);
      }      
      break;
    case ERRO_TELA_MATRICULA:
      beep();
      exibirTela(2);

      mensagemDeErroRecebida = "";
      matricula = 0;
      ordemProducao = 0;
      atividade = 0;
      material = 0;

      delay(3000);

      escreveTela(MATRICULA, 00000);
      telaAtual = TELA_MATRICULA;
      beep();
      break;

    case ERRO_TELA_ORDEM_PRODUCAO:
      beep();
      exibirTela(4);

      mensagemDeErroRecebida = "";
      matricula = 0;
      ordemProducao = 0;
      atividade = 0;
      material = 0;

      delay(3000);

      escreveTela(MATRICULA, 00000);
      telaAtual = TELA_MATRICULA;
      beep();
      break;

    case ERRO_TELA_ATIVIDADE:
      beep();
      exibirTela(6);

      mensagemDeErroRecebida = "";
      matricula = 0;
      ordemProducao = 0;
      atividade = 0;
      material = 0;

      delay(3000);

      escreveTela(MATRICULA, 00000);
      telaAtual = TELA_MATRICULA;
      beep();
      break; 

    case ERRO_TELA_MATERIAL:
      beep();
      exibirTela(6);

      mensagemDeErroRecebida = "";
      matricula = 0;
      ordemProducao = 0;
      atividade = 0;
      material = 0;

      delay(3000);

      escreveTela(MATRICULA, 00000);
      telaAtual = TELA_MATRICULA;
      beep();
      break;   

    case TELA_RASTREABILIDADE1:
      // Exibir tela de rastreabilidade 1
      exibirTela(10);
      // Ler a entrada do teclado

      escreveTela(MATRICULA1, matricula);
      escreveTela(ORDEM_PRODUCAO1, ordemProducao);
      escreveTela(ATIVIDADE1, atividade);
      escreveTela(MATERIAL1, material);

      escreveTela(TENSAO1, tensaoMapeada);
      escreveTela(CORRENTE1, correnteMapeada);
  
      if (tecla == '#') {
        // Tecla "#" pressionada: pausa o processo e avança para a tela RASTREABILIDADE2
        telaAtual = TELA_RASTREABILIDADE2;
        beep();
      } else if (tecla == '*') {
      }
      break;

    case TELA_RASTREABILIDADE2:
      // Exibir tela de rastreabilidade 2
      exibirTela(TELA_RASTREABILIDADE2);
      // Ler a entrada do teclado

      escreveTela(MATRICULA2, matricula);
      escreveTela(ORDEM_PRODUCAO2, ordemProducao);
      escreveTela(ATIVIDADE2, atividade);
      escreveTela(MATERIAL2, material);

      escreveTela(TENSAO2, tensaoMapeada);
      escreveTela(CORRENTE2, correnteMapeada);
      
      if (tecla == '#') {
        // Tecla "#" pressionada: volta para a tela RASTREABILIDADE1
        telaAtual = TELA_RASTREABILIDADE3;
        beep();

      } else if (tecla == 'D') {
        telaAtual = TELA_FINALIZAR_PROCESSO;
        beep();
      }
      break;

    case TELA_RASTREABILIDADE3:
      // Exibir tela de rastreabilidade 2
      exibirTela(TELA_RASTREABILIDADE3);
      // Ler a entrada do teclado

      escreveTela(MATRICULA3, matricula);
      escreveTela(ORDEM_PRODUCAO3, ordemProducao);
      escreveTela(ATIVIDADE3, atividade);
      escreveTela(MATERIAL3, material);

      escreveTela(TENSAO3, tensaoMapeada);
      escreveTela(CORRENTE3, correnteMapeada);
      
      if (tecla == '#') {
        // Tecla "#" pressionada: volta para a tela RASTREABILIDADE1
        telaAtual = TELA_RASTREABILIDADE2;
        beep();
      } else if (tecla == '*') {
      }
      break;
    
    case TELA_FINALIZAR_PROCESSO:
      // Exibir tela de finalizar processo
      exibirTela(TELA_FINALIZAR_PROCESSO);
      // Ler a entrada do teclado
      
      if (tecla == '#') {

        

        // Tecla "#" pressionada: confirma o finalizar processo e avança para a tela CONC
        acaoString = "5";
        enviaValores();

        beep();
        telaAtual = TELA_CONC;

      } else if (tecla == '*') {
        // Tecla "*" pressionada: cancela o finalizar processo e volta para a tela RASTREABILIDADE1
        telaAtual = TELA_RASTREABILIDADE1;
      }
      break;

    case TELA_CONC:
      // Exibir tela de conc
      exibirTela(TELA_CONC);

      delay(5000); // Aguarda 5 segundos antes de avançar para a tela MATRICULA
      escreveTela(MATRICULA, 00000);

      matricula = 0;
      ordemProducao = 0;
      atividade = 0;
      material = 0;

      telaAtual = TELA_MATRICULA;
      beep();
      // ... (implemente a lógica para a tela de conc)
      // Pode ser necessário um contador para atrasar o avanço para a tela MATRICULA após o delay de 5 segundos
      // Certifique-se de definir a variável "acao" corretamente para a próxima ação na tela MATRICULA
      break;
  }
  // recebimento de dados -----------------------------------------------------------

  if (Serial3.available()) {
    
      if (Serial3.available()) {
      String message = Serial3.readStringUntil('\n');
      Serial.println(message); // Print para fins de depuração
      
      if (message.startsWith("Acao:")) {
        int startIndex = message.indexOf("{\"error\":\"");
        if (startIndex != -1) {
          int endIndex = message.indexOf("\"}", startIndex);
          if (endIndex != -1) {
            // Extract the error message from the received message
            mensagemDeErroRecebida = message.substring(startIndex + 10, endIndex);
            Serial.println("ERRO_ESP: " + mensagemDeErroRecebida);
            // Perform actions based on the error, such as displaying on the screen or taking other measures
          }
        } else {
          // Print the received message for debugging
          Serial.println("MENSAGEM: " + message);
        }
      }
    }
  }


  // envio de dados ----------------------------------------------------------------------

  float tensao = analogRead(A0);
  float corrente = analogRead(A1);

  tensaoMapeada =  tensao * 0.0488758553274682; // 0 a 50 - 0.0488758553274682
  correnteMapeada = corrente * 0.2932551319648094; // 0 a 300 - 0.2932551319648094

  // Envia os dados para o ESP8266 via comunicação serial
  Serial3.print("Tensao:");
  Serial3.print(tensaoMapeada);
  Serial3.print("V | Corrente:");
  Serial3.print(correnteMapeada);
  Serial3.println("A");
}