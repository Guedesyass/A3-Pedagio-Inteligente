#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// ======================================
// WIFI
// ======================================

const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";

// ======================================
// RFID
// ======================================

#define SS_PIN 5
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

// ======================================
// SERVO
// ======================================

Servo cancela;

// ======================================
// LEDS
// ======================================

#define LED_VERDE 26
#define LED_VERMELHO 27

// ======================================
// ÂNGULOS
// ======================================

const int FECHADA = 90;
const int ABERTA = 0;

// ======================================
// WEB SERVER
// ======================================

WebServer server(80);

// ======================================
// DASHBOARD — variáveis originais
// (mantidas integralmente)
// ======================================

float valorTotal = 0;
int carrosLiberados = 0;
int pagamentosPendentes = 0;
float tempoTotal = 0;
float tempoMedio = 0;
String statusCancela = "FECHADA";
String historicoHTML = "";
float pedagio = 8.0;

// ======================================
// FLUXO — variáveis originais
// ======================================

unsigned long temposPassagem[20];
int indicePassagem = 0;
int fluxoAtual = 0;
unsigned long ultimoCarro = 0;

// ======================================
// SALDOS
// ======================================

float saldo1 = 1000;
float saldo2 = 5;
float saldo3 = 200;

// ======================================
// NOVOS CAMPOS — necessários para o dashboard
//
// lastPassageSeconds : segundos desde o último
//   carro. O dashboard exibe "Último veículo
//   há X segundos".
//
// Nota: estimatedWait, occupancy e
//   operationalStatus são calculados
//   dinamicamente na função gerarJSON()
//   com base no fluxoAtual, sem precisar
//   de novas variáveis de estado.
// ======================================

unsigned long ultimaPassagemMs = 0; // millis() no momento da última leitura RFID

// ======================================
// CALCULAR FLUXO
// (sem alterações na lógica original)
// ======================================

void calcularFluxo() {

  unsigned long agora = millis();

  if (agora - ultimoCarro > 30000) {
    fluxoAtual = 0;
    return;
  }

  int contador = 0;

  for (int i = 0; i < 20; i++) {
    if (
      temposPassagem[i] != 0 &&
      agora - temposPassagem[i] <= 20000
    ) {
      contador++;
    }
  }

  fluxoAtual = contador;
}

// ======================================
// HISTÓRICO
// (sem alterações na lógica original)
// ======================================

void atualizarHistorico(
  String usuario,
  String status,
  float tempo
) {

  String classe;

  if (status == "APROVADO")       classe = "approved";
  else if (status == "PENDENTE")  classe = "pending";
  else                            classe = "denied";

  historicoHTML =
    "<tr><td>" + usuario +
    "</td><td class='" + classe + "'>" + status +
    "</td><td>" + String(tempo, 1) + "s</td></tr>"
    + historicoHTML;
}

// ======================================
// ABRIR CANCELA
// (sem alterações na lógica original)
// ======================================

void abrirCancela() {

  statusCancela = "ABERTA";

  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_VERDE, HIGH);

  cancela.write(ABERTA);

  unsigned long inicio = millis();

  while (millis() - inicio < 3000) {
    server.handleClient();
    delay(10);
  }

  cancela.write(FECHADA);

  statusCancela = "FECHADA";

  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_VERDE, LOW);
}

// ======================================
// PROCESSAR PAGAMENTO
// (adicionado: registro de ultimaPassagemMs)
// ======================================

void processarPagamento(
  String usuario,
  float &saldo
) {

  unsigned long inicio = millis();

  // Registra o momento da passagem para
  // o campo lastPassageSeconds do dashboard
  ultimaPassagemMs = millis();

  // ======================================
  // APROVADO
  // ======================================

  if (saldo >= pedagio) {

    saldo -= pedagio;
    valorTotal += pedagio;
    carrosLiberados++;

    temposPassagem[indicePassagem] = millis();
    indicePassagem++;
    if (indicePassagem >= 20) indicePassagem = 0;

    calcularFluxo();

    ultimoCarro = millis();

    abrirCancela();

    float tempo = (millis() - inicio) / 1000.0;
    tempoTotal += tempo;
    tempoMedio = tempoTotal / carrosLiberados;

    atualizarHistorico(usuario, "APROVADO", tempo);
  }

  // ======================================
  // PENDENTE
  // ======================================

  else {

    pagamentosPendentes++;

    temposPassagem[indicePassagem] = millis();
    indicePassagem++;
    if (indicePassagem >= 20) indicePassagem = 0;

    calcularFluxo();

    ultimoCarro = millis();

    abrirCancela();

    float tempo = (millis() - inicio) / 1000.0;

    atualizarHistorico(usuario, "PENDENTE", tempo);
  }
}

// ======================================
// HELPERS — derivam os campos novos
// diretamente do fluxoAtual
// ======================================

// Tempo estimado de espera em segundos
int calcularEstimatedWait() {
  if (fluxoAtual == 0 || fluxoAtual <= 2) return 5;
  if (fluxoAtual <= 4)                    return 15;
  return 30;
}

// Percentual de ocupação da cabine
int calcularOccupancy() {
  if (fluxoAtual == 0 || fluxoAtual <= 2) return 30;
  if (fluxoAtual <= 4)                    return 55;
  return 80;
}

// Rótulo de status operacional
String calcularOperationalStatus() {
  if (fluxoAtual == 0)       return "OPERANDO NORMALMENTE";
  if (fluxoAtual <= 2)       return "FLUXO BAIXO";
  if (fluxoAtual <= 4)       return "FLUXO MODERADO";
  return "FLUXO ALTO";
}

// Segundos desde a última passagem RFID.
// Retorna 9999 se nenhuma passagem ocorreu
// desde o boot (dashboard interpreta como "—")
int calcularLastPassageSeconds() {
  if (ultimaPassagemMs == 0) return 9999;
  return (int)((millis() - ultimaPassagemMs) / 1000);
}

// ======================================
// JSON
// Mantém todos os campos originais e
// adiciona os 4 novos campos exigidos
// pelo dashboard público
// ======================================

String gerarJSON() {

  String historicoSeguro = historicoHTML;
  historicoSeguro.replace("\"", "'");

  // Calcula campos derivados no momento
  // da requisição (sempre atualizados)
  int    estimatedWait       = calcularEstimatedWait();
  int    occupancy           = calcularOccupancy();
  String operationalStatus   = calcularOperationalStatus();
  int    lastPassageSeconds  = calcularLastPassageSeconds();

  String json = "{";

  // ---- campos originais (inalterados) ----
  json += "\"valorTotal\":"        + String(valorTotal)         + ",";
  json += "\"approvedCars\":"      + String(carrosLiberados)    + ",";
  json += "\"pendingCars\":"       + String(pagamentosPendentes)+ ",";
  json += "\"avgTime\":"           + String(tempoMedio)         + ",";
  json += "\"gateOpen\":"          + (statusCancela == "ABERTA" ? "true" : "false") + ",";
  json += "\"flow\":"              + String(fluxoAtual)         + ",";
  json += "\"history\":\""        + historicoSeguro            + "\"";

  // ---- campos novos para o dashboard ----
  json += ",\"vehiclesLast20s\":"  + String(fluxoAtual);          // mesmo valor de flow; separado para clareza semântica
  json += ",\"estimatedWait\":"    + String(estimatedWait);
  json += ",\"occupancy\":"        + String(occupancy);
  json += ",\"operationalStatus\":\"" + operationalStatus + "\"";
  json += ",\"lastPassageSeconds\":" + String(lastPassageSeconds);

  json += "}";

  return json;
}

// ======================================
// SETUP
// (sem alterações)
// ======================================

void setup() {

  Serial.begin(115200);

  SPI.begin(18, 19, 23, 5);

  rfid.PCD_Init();

  cancela.setPeriodHertz(50);
  cancela.attach(13, 500, 2400);
  cancela.write(FECHADA);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);

  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_VERDE, LOW);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println(WiFi.localIP());

  server.on("/dados", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", gerarJSON());
  });

  server.begin();
}

// ======================================
// LOOP
// (sem alterações)
// ======================================

void loop() {

  server.handleClient();

  calcularFluxo();

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial())   return;

  String conteudo = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) conteudo += "0";
    conteudo += String(rfid.uid.uidByte[i], HEX);
  }

  conteudo.toLowerCase();

  if      (conteudo == "c20199ee") processarPagamento("FXI2B58", saldo1);
  else if (conteudo == "95d67e05") processarPagamento("QWP7D24", saldo2);
  else if (conteudo == "914d0ea4") processarPagamento("RLA9E75", saldo3);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1500);
}
