#include <Bluepad32.h>

// === Pines de los motores ===
#define IN1A 4  // Motor izquierdo
#define IN1B 15
#define IN2A 22   // Motor derecho
#define IN2B 23

ControllerPtr myController;

// === Función para mover motores ===
void setMotor(int inA, int inB, int speed) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    analogWrite(inA, speed);
    analogWrite(inB, 0);
  } else if (speed < 0) {
    analogWrite(inA, 0);
    analogWrite(inB, -speed);
  } else {
    analogWrite(inA, 0);
    analogWrite(inB, 0);
  }
}

// === Evento cuando se conecta un mando ===
void onConnectedController(ControllerPtr ctl) {
  myController = ctl;
  Serial.println("Control PS4 conectado!");
}

void onDisconnectedController(ControllerPtr ctl) {
  myController = nullptr;
  Serial.println("Control PS4 desconectado!");
}

void setup() {
  Serial.begin(115200);

  // Pines de salida
  pinMode(IN1A, OUTPUT);
  pinMode(IN1B, OUTPUT);
  pinMode(IN2A, OUTPUT);
  pinMode(IN2B, OUTPUT);

  // Iniciar Bluepad32
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();
}

void loop() {
  BP32.update();

  if (myController && myController->isConnected()) {
    // Leer palancas
    int ly = myController->axisY();   // Stick izquierdo (motor izquierdo)
    int ry = myController->axisRY();  // Stick derecho (motor derecho)

    // Leer R2 (valor analógico 0..1023 en PS4)
    int r2 = myController->brake();  // en Bluepad32: brake() = L2, throttle() = R2
    int r2val = myController->throttle(); // throttle() corresponde a R2

    // Mapeo R2: de 0..1023 → 100..255
    int maxSpeed = map(r2val, 0, 1023, 100, 255);

    // Normalizar sticks a rango -maxSpeed..maxSpeed
    int motorLeft  = map(ly, -511, 512, maxSpeed, -maxSpeed);  // invertido (arriba = positivo)
    int motorRight = map(ry, -511, 512, maxSpeed, -maxSpeed);

    // Mover motores
    setMotor(IN1A, IN1B, motorLeft);
    setMotor(IN2A, IN2B, motorRight);

    // Debug
    Serial.print("LY: "); Serial.print(ly);
    Serial.print("  RY: "); Serial.print(ry);
    Serial.print("  R2: "); Serial.print(r2val);
    Serial.print("  maxSpeed: "); Serial.print(maxSpeed);
    Serial.print("  ML: "); Serial.print(motorLeft);
    Serial.print("  MR: "); Serial.println(motorRight);
  }
}
