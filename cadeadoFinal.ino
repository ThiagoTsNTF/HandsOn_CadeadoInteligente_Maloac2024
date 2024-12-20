#include <Wire.h>
#include <Adafruit_PN532.h>
#include <Servo.h>

// Configuração do PN532 (I2C)
#define SDA_PIN 20
#define SCL_PIN 21
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Pinos do Servo, LED RGB, Botões e Buzzer
const int servoPin = 2;
const int redPin = 13;
const int greenPin = 12;
const int bluePin = 11;
const int registerButtonPin = 9; // Botão usado para registrar cartões ou senhas
const int buzzerPin = 10;  // Buzzer

// Botões de senha
const int button1Pin = 6; // Corresponde ao dígito '1'
const int button2Pin = 5; // Corresponde ao dígito '2'
const int button3Pin = 4; // Corresponde ao dígito '3'

// Senha pré-definida inicial: 1, 3, 2, 3
int password[4] = {1, 3, 2, 3};
int userInput[4];
int inputIndex = 0;

// Modo de registro de senha
bool isPasswordRegistrationMode = false;
int newPassword[4];
int newPasswordIndex = 0;

// Servo
Servo servoMotor;
int servoAngle = 90; // Posição inicial do servo

// Lista de UIDs registrados
String registeredUIDs[3];
int registeredCount = 0;

unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 500;
bool isRedOn = false;

// Variáveis para leitura do cartão
bool cardDetected = false;

void setup() {
  Serial.begin(115200);

  // Configuração dos pinos do LED RGB, Botões e Buzzer
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(registerButtonPin, INPUT_PULLUP);
  
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(button3Pin, INPUT_PULLUP);
  
  pinMode(buzzerPin, OUTPUT);

  // Inicializa o servo
  servoMotor.attach(servoPin);
  servoMotor.write(servoAngle);

  // Inicializa o PN532
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

void loop() {
  checkCard();
  checkRegisterButton();
  checkPasswordInput();
  if(!isPasswordRegistrationMode){
    blinkRedLED();
  }
}

// Função para verificar se há um cartão NFC presente e processá-lo
void checkCard() {
  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength,300)) {
    cardDetected = true;
    Serial.println("Cartão detectado!");

    // Converter UID para string
    String cardUID = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (i > 0) cardUID += ":";
      cardUID += String(uid[i], HEX);
    }

    Serial.println("UID do Cartão: " + cardUID);

    // Verificar se o cartão está registrado
    if (isRegistered(cardUID)) {
      Serial.println("Cartão registrado! Acessando...");
      grantAccess(); // Move o servo, muda o LED para verde e toca o buzzer
    } else {
      Serial.println("Cartão não registrado. Pressione o botão para registrar.");
      // Se o botão 9 for pressionado, registramos o cartão
      // (A lógica está na função checkRegisterButton)
    }

    delay(100);
  } else {
    cardDetected = false;
  }
}

// Função para verificar o botão de registro (pino 9)
void checkRegisterButton() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(registerButtonPin);

  if (lastButtonState == HIGH && currentButtonState == LOW) {
    // Botão foi pressionado
    if (cardDetected) {
      // Se temos um cartão detectado, o botão serve para registrar o cartão
      registerCardNow();
    } else {
      // Sem cartão detectado, o botão alterna o modo de registro de senha
      togglePasswordRegistrationMode();
    }
  }

  lastButtonState = currentButtonState;
}

// Função para iniciar ou finalizar o modo de registro de senha
void togglePasswordRegistrationMode() {
  isPasswordRegistrationMode = !isPasswordRegistrationMode;
  if (isPasswordRegistrationMode) {
    setLED(160, 32, 240);
    Serial.println("Modo de registro de senha ativado. Insira 4 dígitos usando os botões (1,2,3).");
    newPasswordIndex = 0;
  } else {
    Serial.println("Modo de registro de senha desativado.");
  }
}

// Função para registrar o cartão atual (caso não esteja registrado)
void registerCardNow() {
  uint8_t uid[7];
  uint8_t uidLength;
  
  // Ler cartão novamente (opcional, pois já temos acima)
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50)) {
    String cardUID = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (i > 0) cardUID += ":";
      cardUID += String(uid[i], HEX);
    }
    
    if (isRegistered(cardUID)) {
      Serial.println("Cartão já está registrado.");
    } else {
      registerCard(cardUID);
    }
  } else {
    Serial.println("Nenhum cartão para registrar.");
  }
}

// Função para verificar a entrada dos botões da senha
void checkPasswordInput() {
  static bool lastStateButton1 = HIGH;
  static bool lastStateButton2 = HIGH;
  static bool lastStateButton3 = HIGH;

  int currentStateButton1 = digitalRead(button1Pin);
  int currentStateButton2 = digitalRead(button2Pin);
  int currentStateButton3 = digitalRead(button3Pin);

  // Checa se o botão 1 foi pressionado
  if (lastStateButton1 == HIGH && currentStateButton1 == LOW) {
    processDigit(1);
  }

  // Checa se o botão 2 foi pressionado
  if (lastStateButton2 == HIGH && currentStateButton2 == LOW) {
    processDigit(2);
  }

  // Checa se o botão 3 foi pressionado
  if (lastStateButton3 == HIGH && currentStateButton3 == LOW) {
    processDigit(3);
  }

  lastStateButton1 = currentStateButton1;
  lastStateButton2 = currentStateButton2;
  lastStateButton3 = currentStateButton3;
}

// Processa um dígito pressionado, dependendo do modo
void processDigit(int digit) {
  if (isPasswordRegistrationMode) {
    // Estamos registrando uma nova senha
    newPassword[newPasswordIndex] = digit;
    newPasswordIndex++;
    Serial.print("Novo dígito da senha: ");
    Serial.println(digit);

    if (newPasswordIndex == 4) {
      // Salva a nova senha
      for (int i = 0; i < 4; i++) {
        password[i] = newPassword[i];
      }
      Serial.println("Nova senha registrada com sucesso!");
      isPasswordRegistrationMode = false; // Sai do modo de registro
    }

  } else {
    // Não estamos no modo de registro de senha, então isso é tentativa de acesso
    userInput[inputIndex] = digit;
    inputIndex++;
    Serial.print("Dígito inserido: ");
    Serial.println(digit);

    // Quando chegamos a 4 dígitos, comparamos a senha
    if (inputIndex == 4) {
      if (checkPassword()) {
        Serial.println("Senha correta! Acessando...");
        grantAccess();
      } else {
        Serial.println("Senha incorreta!");
      }
      // Reseta o índice da tentativa
      inputIndex = 0;
    }
  }
}

// Função para comparar a senha digitada com a senha salva
bool checkPassword() {
  for (int i = 0; i < 4; i++) {
    if (userInput[i] != password[i]) {
      return false;
    }
  }
  return true;
}

// Função para piscar o LED vermelho
void blinkRedLED() {
  if (millis() - lastBlinkTime >= blinkInterval) {
    lastBlinkTime = millis();
    if (isRedOn) {
      setLED(0, 0, 0); // Desliga o LED
    } else {
      setLED(255, 255, 0); // Liga o LED vermelho
    }
    isRedOn = !isRedOn;
  }
}

// Função para configurar o LED RGB
void setLED(int red, int green, int blue) {
  analogWrite(redPin, 255 - red);   // LED RGB inverso
  analogWrite(greenPin, 255 - green);
  analogWrite(bluePin, 255 - blue);
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

// Função para registrar um novo cartão
void registerCard(String uid) {
  if (registeredCount < 3) {
    registeredUIDs[registeredCount] = uid;
    registeredCount++;
    Serial.println("Cartão registrado com sucesso!");
    printRegisteredUIDs();
  } else {
    Serial.println("Limite de registros atingido!");
  }
}

// Função para imprimir os IDs registrados
void printRegisteredUIDs() {
  Serial.println("Cartões registrados no sistema:");
  if (registeredCount == 0) {
    Serial.println("Nenhum cartão registrado.");
  } else {
    for (int i = 0; i < registeredCount; i++) {
      Serial.print("Cartão "); Serial.print(i + 1); Serial.print(": ");
      Serial.println(registeredUIDs[i]);
    }
  }
}

// Função para conceder acesso
void grantAccess() {
  setLED(0, 255, 0); // Muda para verde
  tone(buzzerPin, 1000, 500); // Buzzer toca som de acesso concedido
  servoMotor.write(180); // Move o servo para 180°
  delay(5000); // Aguarda 5 segundos
  servoMotor.write(90); // Retorna o servo para 90°
  tone(buzzerPin, 500, 500); // Buzzer toca som de fechamento
  setLED(255, 0, 0); // Volta para vermelho
}
