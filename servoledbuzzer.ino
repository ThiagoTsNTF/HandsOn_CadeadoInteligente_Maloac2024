#include <Servo.h>

// Instância do servo
Servo servoMotor;

// Pinos do Servo e Buzzer
const int servoPin = 2;      // Pino do servo
const int buttonPin = 9;     // Pino do botão
const int buzzerPin = 10;    // Pino do buzzer

// Pinos do LED RGB
const int redPin = 13;       // Pino do LED (vermelho)
const int greenPin = 12;     // Pino do LED (verde)
const int bluePin = 11;      // Pino do LED (azul)

// Variáveis de controle
int servoAngle = 90;         // Posição inicial do servo
int buttonState = HIGH;      // Estado atual do botão
int lastButtonState = HIGH;  // Último estado do botão

// Constantes de limite de ângulo
const int minAngle = 0;
const int maxAngle = 180;

void setup() {
  // Configura os pinos do botão, buzzer e LED
  pinMode(buttonPin, INPUT_PULLUP); // Botão com pull-up interno
  pinMode(buzzerPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Inicializa o servo
  servoMotor.attach(servoPin);
  servoMotor.write(servoAngle); // Inicia na posição central

  // Inicializa o LED RGB como apagado
  setLED(255, 255, 255); // Apaga todas as cores (inversão lógica)
}

void loop() {
  // Lê o estado atual do botão
  buttonState = digitalRead(buttonPin);

  // Detecta uma mudança de estado (pressionamento do botão)
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Alterna o ângulo do servo
    if (servoAngle == 90) {
      servoAngle = maxAngle; // Vai para 180 graus
      setLED(255, 0, 255);     // LED vermelho (inversão lógica)
    } else if (servoAngle == maxAngle) {
      servoAngle = 90;       // Volta para 90 graus
      setLED(0, 255, 255);     // LED verde (inversão lógica)
    }

    // Move o servo
    servoMotor.write(servoAngle);

    // Emite som no buzzer
    tone(buzzerPin, 1000, 200); // Frequência de 1000 Hz por 200 ms

    // Pequena pausa para evitar múltiplos registros de um único clique
    delay(200);
  }

  // Atualiza o último estado do botão
  lastButtonState = buttonState;
}

// Função para configurar o LED RGB
void setLED(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}