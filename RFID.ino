#include <Wire.h>
#include <Adafruit_PN532.h>

// Configuração do pino do Arduino
#define SDA_PIN 20
#define SCL_PIN 21
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

const int buttonPin = 9;
bool buttonPressed = false;

// Lista de UIDs registrados
String registeredUIDs[3];
int registeredCount = 0;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);

  Serial.println("Iniciando PN532...");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Não foi possível encontrar o PN532. Verifique as conexões!");
    while (1);
  }

  // Exibe informações do firmware
  Serial.print("Encontrado chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Versão do firmware: "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print("."); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Configura o PN532
  nfc.SAMConfig();
  Serial.println("Esperando por um cartão ISO14443A...");
}

void loop(void) {
  uint8_t uid[7];
  uint8_t uidLength;
  Serial.println("Cartões registrados no sistema:");
  printRegisteredUIDs();

  // Verifica se há um cartão presente
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.println("Cartão detectado!");

    // Converter UID para string
    String cardUID = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (i > 0) cardUID += ":"; // Separador entre bytes
      cardUID += String(uid[i], HEX);
    }

    Serial.println("UID do Cartão: " + cardUID);

    // Verificar se o cartão está registrado
    if (isRegistered(cardUID)) {
      Serial.println("Cartão já registrado!");
    } else {
      Serial.println("Cartão não registrado. Aproxime novamente para registrar.");
    }

    // Verificar se o botão está pressionado para registrar o cartão
    if (digitalRead(buttonPin) == LOW && !buttonPressed) {
      buttonPressed = true; // Evita múltiplos registros para o mesmo clique

      // Registrar o cartão se ainda não estiver registrado
      if (!isRegistered(cardUID) && registeredCount < 3) {
        registeredUIDs[registeredCount] = cardUID;
        registeredCount++;
        Serial.println("Cartão registrado com sucesso!");
        Serial.println("Cartões registrados:");
        printRegisteredUIDs(); // Imprime os cartões registrados
      } else if (registeredCount >= 3) {
        Serial.println("Limite de registros atingido!");
      }
    }
  }
  delay(1000);
}

// Função para verificar se o cartão já está registrado
bool isRegistered(String uid) {
  for (int i = 0; i < registeredCount; i++) {
    if (registeredUIDs[i] == uid) {
      return true; // UID já registrado
    }
  }
  return false;
}

// Função para imprimir os IDs registrados
void printRegisteredUIDs() {
  if (registeredCount == 0) {
    Serial.println("Nenhum cartão registrado.");
  } else {
    for (int i = 0; i < registeredCount; i++) {
      Serial.print("Cartão "); Serial.print(i + 1); Serial.print(": ");
      Serial.println(registeredUIDs[i]);
    }
  }
}