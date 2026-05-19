#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// ===== RFID =====
#define SS_PIN 5
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

// ===== SERVO =====
Servo cancela;

// ===== LEDS =====
#define LED_VERDE 26
#define LED_VERMELHO 27

// ===== ÂNGULOS =====
const int FECHADA = 90;
const int ABERTA = 0;

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println("INICIANDO SISTEMA...");

  // ===== SPI RFID =====
  SPI.begin(18, 19, 23, 5);

  // ===== RFID =====
  rfid.PCD_Init();

  // ===== SERVO =====
  cancela.setPeriodHertz(50);
  cancela.attach(13, 500, 2400);

  // Cancela inicia fechada
  cancela.write(FECHADA);

  // ===== LEDS =====
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);

  // Estado inicial:
  // 🔴 ON
  // 🟢 OFF
  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_VERDE, LOW);

  Serial.println("SISTEMA PRONTO");
  Serial.println("APROXIME UMA TAG");
}

void loop() {

  // Verifica nova tag
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Lê tag
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println();
  Serial.println("TAG DETECTADA");

  Serial.print("UID: ");

  // Mostra UID
  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      Serial.print("0");
    }

    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }

  Serial.println();

  // =========================
  // TAG APROXIMADA
  // 🔴 OFF
  // 🟢 ON
  // =========================

  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_VERDE, HIGH);

  Serial.println("ABRINDO CANCELA");

  // Abre cancela
  cancela.write(ABERTA);

  delay(3000);

  // =========================
  // VOLTA AO ESTADO INICIAL
  // 🔴 ON
  // 🟢 OFF
  // =========================

  Serial.println("FECHANDO CANCELA");

  // Fecha cancela
  cancela.write(FECHADA);

  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_VERDE, LOW);

  delay(1000);

  // Finaliza RFID
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}