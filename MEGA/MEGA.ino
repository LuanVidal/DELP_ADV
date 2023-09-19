/*||        //  PROJETO DELP - MEGA  \\         || */

#include <Keypad.h>
#include <time.h>

// Definição de variáveis globais

static unsigned long lastDebounceTime = 0;
static unsigned long debounceDelay = 100;

#define FILTRO 0.03 // TAXA DE VARIAÇÃO
float tensaoAnterior = 0.0;
float correnteAnterior = 0.0;
String message;

String mensagemDeErroRecebida = "";
float correnteMapeada = 0.0;
float tensaoMapeada = 0.0;

bool isConnected = false;
bool aproved = false;
bool timeout = false;
bool pausa = false;
bool restart = false;
bool erro_pause = false;
bool erro_reinico = false;

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

byte pinosLinhas[LINHAS] = {35, 36, 37, 38}; // Pinos conectados às linhas do teclado
byte pinosColunas[COLUNAS] = {31, 32, 33, 34}; // Pinos conectados às colunas do teclado

Keypad keypad = Keypad(makeKeymap(teclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

long int matricula = 0;
long int ordemProducao = 0;
int atividade = 0;
int material = 0;
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
  TELA_CONC,
  ERRO_TIMEOUT
};

unsigned long unix = 0;
time_t hora;


int matricula_P2 = 0;   // Matrícula 2
int ordemProducao_P2 = 0;

String ordemProducaoCompleta = "";
String matriculaCompleta = "";

const int MAX_DIGITOS_MATRICULA = 2;
const int MAX_DIGITOS_MATRICULA2 = 2;

const int MAX_DIGITOS_ORDEM_PRODUCAO = 3;
const int MAX_DIGITOS_ORDEM_PRODUCAO2 = 3;
const int MAX_DIGITOS_ATIVIDADE = 1;
const int MAX_DIGITOS_MATERIAL = 1;

int aux_matricula[5] = {0, 0, 0, 0, 0};
int aux_ordem[7] = {0, 0, 0, 0, 0, 0, 0};

// definicão dos elementos da tela

#define CMD_HEAD1           0x5A
#define CMD_HEAD2           0xA5
#define CMD_WRITE           0x82
#define CMD_READ            0x83

#define MATRICULA           0x20
#define MATRICULA_P2        0X50

#define ORDEM_PRODUCAO      0x21
#define ORDEM_PRODUCAO_P2   0x51

#define ATIVIDADE           0x22
#define MATERIAL            0x23

// definição tela rastreabilidade (1)
#define MATRICULA1           0x24
#define MATRICULA1_2         0X52

#define ORDEM_PRODUCAO1      0x25
#define ORDEM_PRODUCAO1_2    0x53

#define ATIVIDADE1           0x26
#define MATERIAL1            0x27

#define TENSAO1              0x28
#define CORRENTE1            0x29

#define INICIOH1             0x60
#define INICIOM1             0x61
#define INICIOS1             0x62

#define HORA1                0x30
#define MINUTO1              0x31
#define SEGUNDO1             0x80

// definição tela rastreabilidade (2)
#define MATRICULA2           0x32
#define MATRICULA2_2         0x54

#define ORDEM_PRODUCAO2      0x33
#define ORDEM_PRODUCAO2_2    0x55

#define ATIVIDADE2           0x34
#define MATERIAL2            0x35

#define TENSAO2              0x36
#define CORRENTE2            0x37

#define INICIOH2             0x63
#define INICIOM2             0x64
#define INICIOS2             0x65

#define HORA2                0x38
#define MINUTO2              0x39
#define SEGUNDO2             0x81

// definição tela rastreabilidade (3)
#define MATRICULA3           0x40
#define MATRICULA3_2         0x56

#define ORDEM_PRODUCAO3      0x41
#define ORDEM_PRODUCAO3_2    0x57

#define ATIVIDADE3           0x42
#define MATERIAL3            0x43

#define TENSAO3              0x44
#define CORRENTE3            0x45

#define INICIOH3             0x66
#define INICIOM3             0x67
#define INICIOS3             0x68

#define HORA3                0x46
#define MINUTO3              0x47
#define SEGUNDO3             0x82


Tela telaAtual = TELA_INICIAL;
String acaoString = "1";

// AÇÕES

unsigned long lastUpdateTime = 0;
unsigned long updateTimeInterval = 1000; // Atualiza a cada segundo
int receivedMinute = 0;
int receivedHour = 0;
int receivedSecond = 0;

int hoursInicio = 0;
int minutesInicio = 0;
int secondsInicio = 0;

void updateClock() {
  if (millis() - lastUpdateTime >= updateTimeInterval) {
    lastUpdateTime = millis();
    
    receivedSecond++;
    if (receivedSecond >= 60) {
      receivedSecond = 0;
      receivedMinute++;
      if (receivedMinute >= 60) {
        receivedMinute = 0;
        receivedHour++;
        if (receivedHour >= 99) {
          receivedHour =  99;
        }
      }
    }
  }
}

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
    // Concatenar as matrículas e ordens de produção, se houverem

    ordemProducaoCompleta = "";
    matriculaCompleta = "";

    for (int j = 4; j >= 0; j--) {
        matriculaCompleta += String(aux_matricula[j]);
      }
    
    
    for (int i = 6; i >= 0; i--) {
          ordemProducaoCompleta += String(aux_ordem[i]);
    }
    
    String mensagem = "Acao: " + acaoString + ", \"ordemProducao\": " + ordemProducaoCompleta + ", \"matricula\": \"" + matriculaCompleta + "\", \"mac\": \"123-456\", \"atividade\": " + String(atividade) + ", \"material\": " + String(material);
    
    Serial3.println(mensagem); // Envie a mensagem
    
    // Aguarde a confirmação de envio
    while (!Serial3.available()) {
        // Espera até que algum dado esteja disponível na porta Serial3
    }
}


void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial3.begin(115200);
}

void loop() {

  // useabilidade ------------------------------------------------------------------

  char tecla = keypad.getKey();
  static char teclaAnterior = '\0';

  switch (telaAtual) {
    case TELA_INICIAL:
      // Exibir tela inicial
        exibirTela(0);

      // Aguardar até que alguma tecla seja pressionada
      if (isConnected) {
        // Avança para a tela de matrícula

        beep();

        mensagemDeErroRecebida = "Sem erro";
        acaoString = "1" ;//status
        enviaValores();

        telaAtual = TELA_MATRICULA;

        escreveTela(MATRICULA, 000); // Zera o valor da matrícula
        escreveTela(MATRICULA_P2, 000);
        exibirTela(1);
      }
    break;

    case TELA_MATRICULA:
    // Exibir tela de matrícula
    exibirTela(1);

    ordemProducaoCompleta = "";
    matriculaCompleta = "";
    mensagemDeErroRecebida = "";

    lastUpdateTime = 0;
    updateTimeInterval = 1000; // Atualiza a cada segundo
    receivedMinute = 0;
    receivedHour = 0;
    

    if (tecla >= '0' && tecla <= '9') {
      
      if(aux_matricula[4] == 0){
        for (int i = 4; i >= 1; i--) {
          aux_matricula[i]=aux_matricula[i-1]; 
        }

        aux_matricula[0]=(tecla - '0');

        matricula=aux_matricula[0]+(aux_matricula[1]*10)+(aux_matricula[2]*100);
        matricula_P2=aux_matricula[3]+(aux_matricula[4]*10);

        escreveTela(MATRICULA_P2, matricula_P2);
        escreveTela(MATRICULA, matricula);
      }
      
    } else if (tecla == '#' && (millis() - lastDebounceTime) > debounceDelay && matricula_P2 != 0) {
      // Tecla "#" pressionada: avança para a tela de ordem de produção
      lastDebounceTime = millis();

      mensagemDeErroRecebida = "";
      
      beep();

      acaoString = "2";
      enviaValores(); // informar a matricula digitada para o servidor (FAZ O LOGIN DO OPERADOR)

      exibirTela(3);

      ordemProducao = 0000; // Zera o valor da ordem de produção principal
      ordemProducao_P2 = 0000; // Zera o valor da ordem de produção adicional

      escreveTela(ORDEM_PRODUCAO, 0000);
      escreveTela(ORDEM_PRODUCAO_P2, 0000);

      for (int i = 6; i >= 0; i--) {
        aux_ordem[i]=0; 
      }


      telaAtual = TELA_ORDEM_PRODUCAO;

    } else if (tecla == 'D') {
      // Tecla "D" pressionada: corrige as matrículas
      matricula = 0;
      matricula_P2 = 0;
      escreveTela(MATRICULA, 000);
      escreveTela(MATRICULA_P2, 000);
      for (int i = 4; i >= 0; i--) {
        aux_matricula[i]=0; 
      }
    }
    break;

    case TELA_ORDEM_PRODUCAO:
    // Exibir tela de ordem de produção
    exibirTela(3);

//Denner
    // Ler a entrada do teclado
     if (tecla >= '0' && tecla <= '9') {
      
      if(aux_ordem[6] == 0){
        for (int i = 6; i >= 1; i--) {
          aux_ordem[i]=aux_ordem[i-1]; 
        }

        aux_ordem[0]=(tecla - '0');

        ordemProducao=aux_ordem[0]+(aux_ordem[1]*10)+(aux_ordem[2]*100)+(aux_ordem[3]*1000);
        ordemProducao_P2=aux_ordem[4]+(aux_ordem[5]*10)+(aux_ordem[6]*100);

        escreveTela(ORDEM_PRODUCAO_P2, ordemProducao_P2);
        escreveTela(ORDEM_PRODUCAO, ordemProducao);
      }
      
    } else if (tecla == '#' && (millis() - lastDebounceTime) > debounceDelay && ordemProducao_P2 != 0) {
      // Tecla "#" pressionada: avança para a tela de atividade
      lastDebounceTime = millis();

      telaAtual = TELA_ATIVIDADE;
      escreveTela(ATIVIDADE, 00);
      beep();
      exibirTela(5);

    } else if (tecla == 'D') {
      // Tecla "D" pressionada: corrige as ordens de produção
      ordemProducao = 0000; // Zera o valor da ordem de produção principal
      ordemProducao_P2 = 0000; // Zera o valor da ordem de produção adicional
      escreveTela(ORDEM_PRODUCAO, 0000);
      escreveTela(ORDEM_PRODUCAO_P2, 0000);
      for (int i = 6; i >= 0; i--) {
        aux_ordem[i]=0; 
      }

    } else if (tecla == '*') {
      // Tecla "*" pressionada: volta para a tela anterior
      telaAtual = TELA_MATRICULA;
      matricula = 0;
      matricula_P2 = 0;

      escreveTela(MATRICULA, 000);
      escreveTela(MATRICULA_P2, 000);

      mensagemDeErroRecebida = "Sem erro";

      for (int i = 4; i >= 0; i--) {
        aux_matricula[i]=0; 
      }
      
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

      } else if (tecla == '#' && (millis() - lastDebounceTime) > debounceDelay && atividade != 0) {
        // Tecla "#" pressionada: avança para a tela de material

        lastDebounceTime = millis();

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

        ordemProducao = 0000; // Zera o valor da ordem de produção principal
        ordemProducao_P2 = 0000; // Zera o valor da ordem de produção adicional
        
       escreveTela(ORDEM_PRODUCAO, 0000);
       escreveTela(ORDEM_PRODUCAO_P2, 0000);
        for (int i = 6; i >= 0; i--) {
          aux_ordem[i]=0; 
        }

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

      } else if (tecla == '#' && (millis() - lastDebounceTime) > debounceDelay && material != 0) {
        // Tecla "#" pressionada: envia os valores e avança para a tela de aguarde
        lastDebounceTime = millis();

        telaAtual = TELA_AGUARDE;
        beep();

      } else if (tecla == 'D') {
        // Tecla "D" pressionada: corrige o material
        material = 00; // Zera o valor do material
        escreveTela(MATERIAL, 00);

      } else if (tecla == '*') {
        // Tecla "*" pressionada: volta para a tela anterior
        telaAtual = TELA_ATIVIDADE;
        atividade = 00; // Zera o valor da atividade

        escreveTela(ATIVIDADE, 00000);
        exibirTela(3);
      }
      break;

    case TELA_AGUARDE:
      exibirTela(9);

      acaoString = "4";
      enviaValores();
      //inicar trabalho


      if (mensagemDeErroRecebida == "Operador Não Encontrado!") {
        telaAtual = ERRO_TELA_MATRICULA;
        beep();
      }
    
      if (mensagemDeErroRecebida == "Informações Inválidas") {
        telaAtual = ERRO_TELA_MATRICULA;
        beep();
      }

      if (mensagemDeErroRecebida == "Atividade Não Encontrada!"){
        telaAtual = ERRO_TELA_ATIVIDADE;
        beep();
      }

      if (mensagemDeErroRecebida == "Ordem de Produção Não Encontrada!"){
        telaAtual = ERRO_TELA_ORDEM_PRODUCAO;
        beep();
      }

      if (mensagemDeErroRecebida == "Material Não Encontrado!"){
        telaAtual = ERRO_TELA_MATERIAL;
        beep();
      }

      if (timeout) {
        telaAtual = ERRO_TIMEOUT;
        beep();
      }

      if (isConnected == false) {
        telaAtual = ERRO_TIMEOUT;
        beep();
      }

      if (aproved){
        telaAtual = TELA_RASTREABILIDADE2;
        beep();
      }
      break;


    case ERRO_TIMEOUT:
      beep();
      exibirTela(15);

      if(erro_pause){
        delay(3000);
        telaAtual = TELA_RASTREABILIDADE2;
        erro_pause = false;
      }

      if(erro_reinico){
        delay(3000);
        telaAtual = TELA_RASTREABILIDADE3;
        erro_reinico = false;

      } else {
        mensagemDeErroRecebida = "";

        matricula = 0;
        ordemProducao = 0; // Zera o valor da ordem de produção principal
        ordemProducao_P2 = 0; // Zera o valor da ordem de produção adicional
        atividade = 0;
        material = 0;

        aproved = false;

        ordemProducaoCompleta = "";
        matriculaCompleta = "";

        delay(3000);

        matricula = 0;
        matricula_P2 = 0;
        escreveTela(MATRICULA, 000);
        escreveTela(MATRICULA_P2, 000);

        for (int i = 4; i >= 0; i--) {
          aux_matricula[i]=0; 
        }

        telaAtual = TELA_MATRICULA;
        beep();
      }
      break;

    case ERRO_TELA_MATRICULA:
      beep();
      exibirTela(2);

      mensagemDeErroRecebida = "";

      matricula = 0;
      ordemProducao = 0; // Zera o valor da ordem de produção principal
      ordemProducao_P2 = 0; // Zera o valor da ordem de produção adicional
      atividade = 0;
      material = 0;

      aproved = false;

      ordemProducaoCompleta = "";
      matriculaCompleta = "";

      delay(3000);

      matricula = 0;
      matricula_P2 = 0;
      escreveTela(MATRICULA, 000);
      escreveTela(MATRICULA_P2, 000);

      for (int i = 4; i >= 0; i--) {
        aux_matricula[i]=0; 
      }

      telaAtual = TELA_MATRICULA;
      beep();
      break;

    case ERRO_TELA_ORDEM_PRODUCAO:
      beep();
      exibirTela(4);

      mensagemDeErroRecebida = "Sem erro";
      ordemProducaoCompleta = "";
      matriculaCompleta = "";

      matricula = 0;
      ordemProducao = 0000; // Zera o valor da ordem de produção principal
      ordemProducao_P2 = 0000; // Zera o valor da ordem de produção adicional
      atividade = 0;
      material = 0;

      delay(3000);

      matricula = 0;
      matricula_P2 = 0;

      aproved = false;

      escreveTela(MATRICULA, 000);
      escreveTela(MATRICULA_P2, 000);

      for (int i = 4; i >= 0; i--) {
        aux_matricula[i]=0; 
      }

      telaAtual = TELA_MATRICULA;

      beep();
      break;

    case ERRO_TELA_ATIVIDADE:
      beep();
      exibirTela(6);

      mensagemDeErroRecebida = "sem erro";
      ordemProducaoCompleta = "";
      matriculaCompleta = "";

      matricula = 0;
      ordemProducao = 0000; // Zera o valor da ordem de produção principal
      ordemProducao_P2 = 0000; // Zera o valor da ordem de produção adicional
      atividade = 0;
      material = 0;

      aproved = false;

      delay(3000);

      matricula = 0;
      matricula_P2 = 0;

      escreveTela(MATRICULA, 000);
      escreveTela(MATRICULA_P2, 000);
      for (int i = 4; i >= 0; i--) {
        aux_matricula[i]=0; 
      }

      telaAtual = TELA_MATRICULA;

      beep();
      break; 

    case ERRO_TELA_MATERIAL:
      beep();
      exibirTela(8);

      mensagemDeErroRecebida = "Sem erro";
      ordemProducaoCompleta = "";
      matriculaCompleta = "";

      matricula = 0;
      ordemProducao = 0000; // Zera o valor da ordem de produção principal
      ordemProducao_P2 = 0000; // Zera o valor da ordem de produção adicional
      atividade = 0;
      material = 0;

      delay(3000);

      aproved = false;

      matricula = 0;
      matricula_P2 = 0;
      escreveTela(MATRICULA, 000);
      escreveTela(MATRICULA_P2, 000);
      

      for (int i = 4; i >= 0; i--) {
        aux_matricula[i]=0; 
      }

      telaAtual = TELA_MATRICULA;

      beep();
      break;   

    case TELA_RASTREABILIDADE1:

      // Exibir tela de rastreabilidade 1
      exibirTela(10);
      // Ler a entrada do teclado

      escreveTela(MATRICULA1, matricula);
      escreveTela(MATRICULA1_2, matricula_P2);

      escreveTela(ORDEM_PRODUCAO1, ordemProducao);
      escreveTela(ORDEM_PRODUCAO1_2, ordemProducao_P2);

      escreveTela(ATIVIDADE1, atividade);
      escreveTela(MATERIAL1, material);

      escreveTela(TENSAO1, tensaoAnterior);
      escreveTela(CORRENTE1, correnteAnterior);

      escreveTela(INICIOH1, hoursInicio);
      escreveTela(INICIOM1, minutesInicio);
      escreveTela(INICIOS1, secondsInicio);
      
      if (tecla == '#' && (millis() - lastDebounceTime) > debounceDelay) {
        // Tecla "#" pressionada: pausa o processo e avança para a tela RASTREABILIDADE2
        lastDebounceTime = millis();

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
      escreveTela(MATRICULA2_2, matricula_P2);

      escreveTela(ORDEM_PRODUCAO2, ordemProducao);
      escreveTela(ORDEM_PRODUCAO2_2, ordemProducao_P2);

      escreveTela(ATIVIDADE2, atividade);
      escreveTela(MATERIAL2, material);

      escreveTela(TENSAO2, tensaoAnterior);
      escreveTela(CORRENTE2, correnteAnterior);

      escreveTela(INICIOH2, hoursInicio);
      escreveTela(INICIOM2, minutesInicio);
      escreveTela(INICIOS2, secondsInicio);
      
      updateClock();
      
      escreveTela(HORA2, receivedHour);
      escreveTela(MINUTO2, receivedMinute);
      escreveTela(SEGUNDO2, receivedSecond);

      
      if (tecla == '#' && (millis() - lastDebounceTime) > debounceDelay) {
        // Tecla "#" pressionada: volta para a tela RASTREABILIDADE1
        lastDebounceTime = millis();

        escreveTela(HORA3, receivedHour);
        escreveTela(MINUTO3, receivedMinute);
        escreveTela(SEGUNDO3, receivedSecond);

        escreveTela(INICIOH3, hoursInicio);
        escreveTela(INICIOM3, minutesInicio);
        escreveTela(INICIOS3, secondsInicio);

        beep();

        acaoString = "6";
        enviaValores();

        if(message.indexOf("Requisicao bem sucedida:") != -1){
          telaAtual = TELA_RASTREABILIDADE3;
        }

        if (message.indexOf("Erro de timeout na requisicao HTTP.") != -1) {
          telaAtual = ERRO_TIMEOUT;
          erro_pause = true;
        }

      } else if (tecla == '*') {

        beep();

        acaoString = "6";
        enviaValores();

        telaAtual = TELA_FINALIZAR_PROCESSO;
      }
      break;

    case TELA_RASTREABILIDADE3:
      // Exibir tela de rastreabilidade 2
      exibirTela(TELA_RASTREABILIDADE3);
      // Ler a entrada do teclado

      escreveTela(MATRICULA3, matricula);
      escreveTela(MATRICULA3_2, matricula_P2);

      escreveTela(ORDEM_PRODUCAO3, ordemProducao);
      escreveTela(ORDEM_PRODUCAO3_2, ordemProducao_P2);

      escreveTela(ATIVIDADE3, atividade);
      escreveTela(MATERIAL3, material);

      escreveTela(TENSAO3, tensaoAnterior);
      escreveTela(CORRENTE3, correnteAnterior);

      
      if (tecla == '#' && (millis() - lastDebounceTime) > debounceDelay) {
        // Tecla "#" pressionada: volta para a tela RASTREABILIDADE1
        lastDebounceTime = millis();

        beep();

        acaoString = "7";
        enviaValores();

        if(message.indexOf("Requisicao bem sucedida:") != -1){
          telaAtual = telaAtual = TELA_RASTREABILIDADE2;;
        }

        if (message.indexOf("Erro de timeout na requisicao HTTP.") != -1) {
          telaAtual = ERRO_TIMEOUT;
          erro_reinico = true;
        }
        
      } else if (tecla == '*') {
      }
      break;
    
    case TELA_FINALIZAR_PROCESSO:
      // Exibir tela de finalizar processo
      exibirTela(TELA_FINALIZAR_PROCESSO);
      // Ler a entrada do teclado
      
      if (tecla == '#' && (millis() - lastDebounceTime) > debounceDelay) {
        lastDebounceTime = millis();
        // Tecla "#" pressionada: confirma o finalizar processo e avança para a tela CONC
        acaoString = "5";
        enviaValores();

        beep();
        telaAtual = TELA_CONC;

      } else if (tecla == '*') {
        // Tecla "*" pressionada: cancela o finalizar processo e volta para a tela RASTREABILIDADE1
        telaAtual = TELA_RASTREABILIDADE3;
      }
      break;

    case TELA_CONC:
      // Exibir tela de conc
      exibirTela(TELA_CONC);

      delay(5000); // Aguarda 5 segundos antes de avançar para a tela MATRICULA

      matricula = 0;
      matricula_P2 = 0;
      mensagemDeErroRecebida = "Sem erro";

      escreveTela(MATRICULA, 000);
      escreveTela(MATRICULA_P2, 000);

      for (int i = 4; i >= 0; i--) {
        aux_matricula[i]=0; 
      }

      ordemProducao = 0000; // Zera o valor da ordem de produção principal
      ordemProducao_P2 = 0000; // Zera o valor da ordem de produção adicional
      atividade = 0;
      material = 0;

      aproved = false;

      telaAtual = TELA_MATRICULA;
      beep();
      break;
  }

  // recebimento de dados -----------------------------------------------------------

  if (Serial3.available()) {
    message = Serial3.readStringUntil('\n');
      Serial.println(message); // Print para fins de depuração
      
      if (message.startsWith("{\"error\":\"")) {
        int startIndex = message.indexOf("{\"error\":\"");
        if (startIndex != -1) {
          int endIndex = message.indexOf("\"}", startIndex);
          if (endIndex != -1) {
            // Extract the error message from the received message
            mensagemDeErroRecebida = message.substring(startIndex + 10, endIndex);
            Serial.println("ERRO_ESP:" + mensagemDeErroRecebida);
            // Perform actions based on the error, such as displaying on the screen or taking other measures
          }
        } else {
          // Print the received message for debugging
          Serial.println("MENSAGEM: " + message);
        }
      }

    if (message.indexOf("Conectado ao Broker com sucesso!") != -1) {
      isConnected = true; // Se a mensagem de conexão bem-sucedida for recebida, atualize o status de conexão
    }

    if (message.indexOf("Nao foi possivel se conectar ao broker.") != -1) {
      isConnected = false;
    }

    if (message.indexOf("Processo Iniciado Sem Erros") != -1) {
      aproved = true;
    }

    if (message.indexOf("Timeout de conexão atingido.") != -1) {
      timeout = true;
    }

    if (message.indexOf("Processo Pausado Sem Erros") != -1) {
      pausa = true;
    }

    if (message.indexOf("Processo Reiniciado Sem Erros") != -1) {
      restart = true;
    }

    int delimiterIndex = message.indexOf('-');

    if (delimiterIndex != -1) {
        // Separe a string em horas, minutos e segundos
      String hourString = message.substring(0, delimiterIndex);
      message.remove(0, delimiterIndex + 1); // Remove a parte da string que já foi processada
      delimiterIndex = message.indexOf('-');
        
      if (delimiterIndex != -1) {
        String minuteString = message.substring(0, delimiterIndex);
        String secondString = message.substring(delimiterIndex + 1);
          
          // Converta as strings em números inteiros
        hoursInicio = hourString.toInt() - 3;
        minutesInicio = minuteString.toInt();
        secondsInicio = secondString.toInt();
      }
    }

  }

  // envio de dados ----------------------------------------------------------------------

  float tensao = analogRead(A1);
  float corrente = analogRead(A1);

  tensaoMapeada =  tensao * 0.0977517106549365; // 0 a 100 - 0.0488758553274682
  correnteMapeada = corrente * 0.5865102639296188; // 0 a 600 - 0.2932551319648094
  
  if (tensaoMapeada <= 100) {
    // Aplicar o filtro com margem de 3%
    if (tensaoAnterior + (100 * FILTRO) < tensaoMapeada || tensaoAnterior - (100 * FILTRO) > tensaoMapeada ) {
      tensaoAnterior = tensaoMapeada;
    }
    Serial3.print("Tensao:");
    Serial3.print(tensaoAnterior);
    Serial3.print("V |");

  } else {
    tensaoAnterior = 100;
    Serial3.print("Tensao:");
    Serial3.print(tensaoAnterior);
    Serial3.print("V |");

  }

  if (tensaoMapeada <= 0) {
    tensaoAnterior = 0;
    Serial3.print("Tensao:");
    Serial3.print(tensaoAnterior);
    Serial3.print("V |");
  }

  if (correnteMapeada <= 600) {
    // Aplicar o filtro com margem de 3%
    if (correnteAnterior + (600 * FILTRO) < correnteMapeada || correnteAnterior - (600 * FILTRO) > correnteMapeada ) {
      correnteAnterior = correnteMapeada;
    } 
 
    Serial3.print("Corrente:");
    Serial3.print(correnteAnterior);
    Serial3.println("A");

  } else {
    correnteAnterior = 600;
    Serial3.print("Corrente:");
    Serial3.print(correnteAnterior);
    Serial3.println("A");
  }

  if (correnteMapeada <= 0) {
    correnteAnterior = 0;
    Serial3.print("Corrente:");
    Serial3.print(correnteAnterior);
    Serial3.println("A");
  }

  delay(20);
  // Envia os dados para o ESP8266 via comunicação serial
}