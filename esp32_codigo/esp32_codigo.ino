#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// ==========================
// WIFI
// ==========================

const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";

// ==========================
// RFID
// ==========================

#define SS_PIN 5
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

// ==========================
// SERVO
// ==========================

#define SERVO_PIN 13

Servo cancela;

// ==========================
// LEDS
// ==========================

#define LED_GREEN 27
#define LED_RED 26

// ==========================
// WEB SERVER
// ==========================

WebServer server(80);

// ==========================
// VARIÁVEIS DO DASHBOARD
// ==========================

float valorTotal = 0;

int carrosLiberados = 0;

int pagamentosNegados = 0;

float tempoTotal = 0;

float tempoMedio = 0;

String statusCancela = "FECHADA";

String historicoHTML = "";

float pedagio = 8.0;

// ==========================
// SALDOS
// ==========================

float saldo1 = 1000;
float saldo2 = 5;
float saldo3 = 200;

// ==========================
// ABRIR CANCELA
// ==========================

void abrirCancela(){

  statusCancela = "ABERTA";

  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);

  cancela.write(90);

  delay(3000);

  cancela.write(0);

  statusCancela = "FECHADA";

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);

}

// ==========================
// HISTÓRICO
// ==========================

void atualizarHistorico(
  String usuario,
  String status,
  float tempo
){

  String classe =
  status == "APROVADO"
  ? "approved"
  : "denied";

  historicoHTML =
  "<tr><td>" + usuario +
  "</td><td class='" + classe +
  "'>" + status +
  "</td><td>" +
  String(tempo,1) +
  "s</td></tr>"
  + historicoHTML;

}

// ==========================
// PROCESSAMENTO
// ==========================

void processarPagamento(
  String usuario,
  float &saldo
){

  unsigned long inicio =
  millis();

  if(saldo >= pedagio){

    saldo -= pedagio;

    valorTotal += pedagio;

    carrosLiberados++;

    abrirCancela();

    float tempo =
    (millis() - inicio)
    / 1000.0;

    tempoTotal += tempo;

    tempoMedio =
    tempoTotal /
    carrosLiberados;

    atualizarHistorico(
      usuario,
      "APROVADO",
      tempo
    );

  }else{

    pagamentosNegados++;

    digitalWrite(LED_GREEN, LOW);
    piscarLedVermelho();

    float tempo =
    (millis() - inicio)
    / 1000.0;

    atualizarHistorico(
      usuario,
      "NEGADO",
      tempo
    );

  }

}

// ==========================
// GERAR JSON
// ==========================

String gerarJSON(){

  String json = "{";

  json += "\"valorTotal\":";
  json += valorTotal;
  json += ",";

  json += "\"approvedCars\":";
  json += carrosLiberados;
  json += ",";

  json += "\"deniedCars\":";
  json += pagamentosNegados;
  json += ",";

  json += "\"avgTime\":";
  json += tempoMedio;
  json += ",";

  json += "\"gateOpen\":";

  if(statusCancela == "ABERTA"){

    json += "true";

  }else{

    json += "false";
  }

  json += ",";

  json += "\"flow\":";
  json += random(20,90);
  json += ",";

  json += "\"history\":\"";
  json += historicoHTML;
  json += "\"";

  json += "}";

  return json;

}

// ==========================
// SETUP
// ==========================

void piscarLedVermelho(){

  for(int i = 0; i < 3; i++){

    digitalWrite(LED_RED, HIGH);

    delay(300);

    digitalWrite(LED_RED, LOW);

    delay(300);

  }

  // Mantém vermelho ligado após piscar
  digitalWrite(LED_RED, HIGH);

}

void setup(){

  Serial.begin(115200);

  SPI.begin();

  rfid.PCD_Init();

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  digitalWrite(LED_RED, HIGH);

  cancela.attach(SERVO_PIN);

  cancela.write(0);

  // ==========================
  // WIFI
  // ==========================

  WiFi.begin(ssid, password);

  Serial.print("Conectando");

  while(
    WiFi.status()
    != WL_CONNECTED
  ){

    delay(500);
    Serial.print(".");

  }

  Serial.println("");
  Serial.println("WiFi conectado");

  // ==========================
  // MOSTRAR IP
  // ==========================

  Serial.print("IP do ESP32: ");

  Serial.println(
    WiFi.localIP()
  );

  // ==========================
  // ROTA /dados
  // ==========================

  server.on("/dados", [](){

    server.sendHeader(
      "Access-Control-Allow-Origin",
      "*"
    );

    server.send(
      200,
      "application/json",
      gerarJSON()
    );

  });

  server.begin();

}

// ==========================
// LOOP
// ==========================

void loop(){

  server.handleClient();

  if(
    !rfid.PICC_IsNewCardPresent()
  ){
    return;
  }

  if(
    !rfid.PICC_ReadCardSerial()
  ){
    return;
  }

  String conteudo = "";

  for(
    byte i = 0;
    i < rfid.uid.size;
    i++
  ){

    conteudo +=
    String(
      rfid.uid.uidByte[i],
      HEX
    );

  }

  conteudo.toLowerCase();

  Serial.println(conteudo);

  /*
    TROCAR PELOS UIDS REAIS
  */

  if(conteudo == "C20199EE"){

    processarPagamento(
      "FXI2B58",
      saldo1
    );

  }

  else if(conteudo == "95D67E05"){

    processarPagamento(
      "QWP7D24",
      saldo2
    );

  }

  else if(conteudo == "914D0EA4"){

    processarPagamento(
      "RLA9E75",
      saldo3
    );

  }

  rfid.PICC_HaltA();

}
