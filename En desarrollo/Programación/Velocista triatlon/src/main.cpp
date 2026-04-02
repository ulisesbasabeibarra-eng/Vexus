#include <Arduino.h>
#include <BluetoothSerial.h>

#define D1 39
#define D2 34
#define D3 35
#define D4 32
#define D5 33
#define D6 25
#define D7 26
#define D8 27
#define SENSORES_TOTAL 8

// ===== Motores ====
#define IN1A 22
#define IN1B 23 //bts 1
#define IN2A 4
#define IN2B 15 //bts 2
int VMAX = 180; //APROX RPM
int VBASE = 160; //APROX RPM
int VMIN = 140; //APROX RPM

#define led 2
#define boton 17

// ===== PWM's =====
int PWM1 = VBASE;  //pwm de la izquierda
int PWM2 = VBASE;  //pwm de la derecha

// Configuración de PWM para control de motores
const int frequency = 1000;
const int resolution = 8;

// ===== Canales PWM del ESP32 =====
const int ledChannel = 0;
const int ledChannel1 = 1;
const int ledChannel2 = 2;
const int ledChannel3 = 3;

// ===== Constantes y vaiables PID =====
float kp = 2;     //proporcional-presente
float ki = 0.01;  //integral-pasado
float kd = 0.5;   //derivativo-futuro
float error = 0;
float prevError = 0;
float integral = 0;
float derivative = 0;
float outputPID = 0;
float lastError = 0;
float setpoint = 3500;
int   correccion = 0;

// ========= Sensores =========
const int sensores[8] = {D1, D2, D3, D4, D5, D6, D7, D8}; // pines de la regleta - se puede poner esto en vez del define -
int valor_blanco[8];
int valor_negro[8];
int valor_umbrales[8];
bool valor_binario[8];

// ====== Precaución ======
bool esperando_inicio = false; 
bool activo = false;
ulong tiempo_ini = 0;
ulong prev_time, current_time, tiempo_trans, time_luz;
const uint16_t tiempo_led = 3000;
const uint16_t tiempo_comp = 5000;

/*left = izq
  rigth = der*/

// ===== INICIALIZACION DEL BLUETOOTH =====
BluetoothSerial SerialBT;

// ===== FUNCIONES =====
void motores(int izq, int der) {  //0 hasta 255 adelante 0 hasta -255 atras

  if (izq >= 0) {
    ledcWrite(ledChannel, izq);
    ledcWrite(ledChannel1, 0);  //analog
  } else {
    izq = izq * (-1);
    ledcWrite(ledChannel1, izq);
    ledcWrite(ledChannel, 1);
  }
  //motor derecho//
  if (der >= 0) {
    ledcWrite(ledChannel2, der);
    ledcWrite(ledChannel3, 0);
  } else {
    der = der * (-1);
    ledcWrite(ledChannel3, der);
    ledcWrite(ledChannel2, 1);
  }
}

int calcularPID(int lectura) {
  error = setpoint - lectura;
  integral += error;
  derivative = error - lastError;
  lastError = error;
  return kp * error + ki * integral + kd * derivative;
}

void printArrayBT(const char *label, const int *arr, int n){
  SerialBT.print(label);
  SerialBT.print(": ");
  for (int i = 0; i < n; i++){
    SerialBT.print(arr[i]);
    if (i < n - 1)
      SerialBT.print(", ");
  }
  SerialBT.println();
}

void calibrar_sensores(){ // si se presiona una vez el boton empieza a leer valores del boton blanco, si se presiona 2 lee negros
  digitalWrite(led, HIGH); // LED ON: inicio calibración

  // Presionar el boton para medir blanco (creo que se debe mantener presionado - no estoy seguro)
  while (digitalRead(boton) == 0){}
  delay(10);
  for (int x = 0; x < 8; x++){
    int valor_prom = 0;
    for (int i = 0; i < 10; i++)
      valor_prom += analogRead(sensores[x]);
    valor_blanco[x] = valor_prom / 10;
  }
  SerialBT.println("[CAL] Lectura BLANCO:");
  printArrayBT("blanco", valor_blanco, 8);

  while (digitalRead(boton) == 1){}
  delay(10);
  while (digitalRead(boton) == 0){} // se usa en 0 por el pull up - seria un 1 logico -
  delay(10);
  for (int x = 0; x < 8; x++){
    int valor_prom = 0;
    for (int i = 0; i < 10; i++)
      valor_prom += analogRead(sensores[x]);
    valor_negro[x] = valor_prom / 10;
  }
  SerialBT.println("[CAL] Lectura BLANCO:");
  printArrayBT("blanco", valor_negro, 8);
  // Calcular e imprimir umbrales
  for (int x = 0; x < 8; x++){
    valor_umbrales[x] = (valor_blanco[x] + valor_negro[x]) / 2;
  }
  SerialBT.println("[CAL] UMBRALES calculados:");
  printArrayBT("umbral", valor_umbrales, 8);

  digitalWrite(led, LOW); // LED OFF: fin calibración

  while (digitalRead(boton) == LOW){}
  delay(10);
}

void precaucion() {
    time_luz = tiempo_led;
    current_time = millis(); 
    tiempo_trans = current_time - tiempo_ini;

    //se enciende entre 3 y 5 segundos
    // Condición: (tiempo transcurrido mayor o igual a 3000ms) Y (tiempo transcurrido menor a 5000ms)
    if (tiempo_trans >= time_luz && tiempo_trans < tiempo_comp){
        digitalWrite(led, HIGH);
    } else {
        digitalWrite(led, LOW);
    }

    if (tiempo_trans >=  tiempo_comp) {
        // La cuenta regresiva terminó
        esperando_inicio = false; // Desactiva el estado de espera
        activo = true;            // Activa el estado de batalla
        digitalWrite(led, LOW);   // Apaga la luz final
    }

    motores(0, 0); 
}

int leer_linea(){
 int valores[SENSORES_TOTAL];
 long suma = 0; // Usar long para evitar desbordamiento en sumas grandes
 long suma_total = 0;

 for (int i = 0; i < SENSORES_TOTAL; i++) {
      // Usamos el puntero a los pines miembro _pinesSensores
      // Leemos los pines como digitales (0 o 1). Si fueran analógicos, sería analogRead().
      valores[i] = analogRead(sensores[i]); 
      suma += valores[i];
      suma_total += (long)valores[i] * i * 1000;
    }

if (suma == 0){
      return lastError; // Usar _ultimoError (variable miembro)
    }

    // Posición centrada es: (NUM_SENSORES - 1) * 1000 / 2 = 7 * 1000 / 2 = 3500
    // La posición retornada será (sumaPonderada / suma) - 3500
    int posicion = (int)(suma_total / suma) - ((SENSORES_TOTAL - 1) * 500); 
    return posicion;
  }

void setup(){
  Serial.begin(115200);

  // declaracion de pines regleta
  for (int i = 0; i < 8; i++){
    pinMode(sensores[i], INPUT);
  }

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  pinMode(boton, INPUT_PULLUP);

  ledcSetup(ledChannel, frequency, resolution);
  ledcAttachPin(IN1A, ledChannel);

  ledcSetup(ledChannel1, frequency, resolution);
  ledcAttachPin(IN1B, ledChannel1);

  ledcSetup(ledChannel2, frequency, resolution);
  ledcAttachPin(IN2A, ledChannel2);

  ledcSetup(ledChannel3, frequency, resolution);
  ledcAttachPin(IN2B, ledChannel3);

  calibrar_sensores();
}

void loop(){

  if (!activo && !esperando_inicio){
    if (digitalRead(boton) == LOW){
      tiempo_ini = millis(); // CRÍTICO: Asignar el tiempo de inicio UNA SOLA VEZ
      esperando_inicio = true;
    }
    return;
  }

  if (esperando_inicio){
    precaucion();
    return;
  }

  if (activo){

    int posicion = leer_linea();

    error = setpoint - posicion;

    derivative = error - lastError;
    integral += error;
    lastError = error;
    outputPID = (kp * error) + (ki * integral) + (kd * derivative);

    int velocidad_izq = VBASE - outputPID;
    int velocidad_der = VBASE +  outputPID;

    motores(velocidad_izq, velocidad_der);

    if (SerialBT.available() > 0){
      char dato = SerialBT.read();
      switch (dato){
      case '1':
        kp += 0.05f;
        break;
      case '2':
        kp -= 0.05f;
        break;

      case '3':
        kd += 0.1f;
        break;
      case '4':
        kd -= 0.1f;
        break;

      case '5':
        VBASE += 10;
        SerialBT.print("la velocidad base ahora es:");
        SerialBT.println(VBASE);
        // VBASE = constrain(baseSpeed, 0, 255);
        break;
      case '6':
        VBASE -= 10;
        SerialBT.print("la velocidad base ahora es:");
        SerialBT.println(VBASE);
        // VBASE = constrain(baseSpeed, 0, 255);
        break;

      default:
        break;
      }
    }
  }
}
