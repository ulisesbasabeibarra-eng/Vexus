#include <Bluepad32.h>

// === Pines de los motores ===
#define IN1A 39  //15  // Motor izquierdo
#define IN1B 14  //4
#define IN2A  4  //23 // Motor derecho
#define IN2B  5  //22

#define rele 38 // 5

ControllerPtr myController = nullptr;

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
  Serial.println("¡Control conectado!");
}

void onDisconnectedController(ControllerPtr ctl) {
  if (myController == ctl) {
    myController = nullptr;
    Serial.println("¡Control desconectado!");
  }
}

void setup() {
  Serial.begin(115200);

  // Pines de motores como salida
  pinMode(IN1A, OUTPUT);
  pinMode(IN1B, OUTPUT);
  pinMode(IN2A, OUTPUT);
  pinMode(IN2B, OUTPUT);

  // Pin del relé como salida
  pinMode(rele, OUTPUT);
  digitalWrite(rele, LOW);

  // Iniciar Bluepad32
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();
}

void loop() {
  BP32.update();

  if (myController && myController->isConnected()) {
    // Leer palancas
    int ly = myController->axisY();   // Stick izquierdo
    int ry = myController->axisRY();  // Stick derecho

    // Leer R2 (throttle)
    int r2val = myController->throttle(); // R2 en Bluepad32

    // Mapeo de velocidad según R2
    int maxSpeedleft  = map(r2val, 0, 1023, 120, 250);
    int maxSpeedright = map(r2val, 0, 1023, 100, 230);

    int motorLeft  = map(ly, -511, 512, -maxSpeedleft, maxSpeedleft);
    int motorRight = map(ry, -511, 512, -maxSpeedright, maxSpeedright);

    // Zona muerta para evitar vibraciones
    if (abs(motorLeft) < 15){ 
     motorLeft = 0;}
    if (abs(motorRight) < 15) {
      motorRight = 0;}

    // Mover motores
    setMotor(IN1A, IN1B, motorLeft);
    setMotor(IN2A, IN2B, motorRight);

    // Control del Relé con L1
    if (myController->l1()) {
      digitalWrite(rele, HIGH); // Cambia a Tensión 2 (NO) al presionar L1 
      Serial.println("L1 presionado: Relé ACTIVADO");
    } else {
      digitalWrite(rele, LOW);  // Vuelve a Tensión 1 (NC) al soltar L1
    }

    // Debugr
    Serial.print("  R2: "); Serial.print(r2val);
// Serial.print("  maxSpeed: "); Serial.print(maxSpeed);
    Serial.print("  ML: "); Serial.print(motorLeft);
    Serial.print("  MR: "); Serial.println(motorRight);
  }
}
