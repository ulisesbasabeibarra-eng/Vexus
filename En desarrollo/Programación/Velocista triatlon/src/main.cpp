#include <Arduino.h>
#include <BluetoothSerial.h>
#include <QTRSensors.h>

// ===== INICIALIZACION DEL BLUETOOTH Y LIBRERIA QTR =====
BluetoothSerial SerialBT;
QTRSensors qtr;

#define D1 39
#define D2 34
#define D3 35
#define D4 32
#define D5 33
#define D6 25
#define D7 26
#define D8 27

const uint8_t SENSORES_TOTAL = 8;
uint16_t sensorValues[SENSORES_TOTAL];

// ===== Motores ====
#define IN1A 22
#define IN1B 23 //bts 1
#define IN2A 4
#define IN2B 15 //bts 2
int VMAX = 60; //APROX RPM
int VBASE = 50; //APROX RPM
int VMIN = 30; //APROX RPM

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
float kp = 0.01;     //proporcional-presente
float ki = 0;     //integral-pasado
float kd = 0.5;   //derivativo-futuro
float error = 0;
float prevError = 0;
float integral = 0;
float derivative = 0;
float outputPID = 0;
float lastError = 0;
float setpoint = 5500;
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

// ===== FUNCIONES =====
void motores(int izq, int der) {  //0 hasta 255 adelante 0 hasta -255 atras

  if (izq >= 0) {
    if(izq > 255){
      izq = 255;
    }
    ledcWrite(ledChannel, izq);
    ledcWrite(ledChannel1, 0);  //analog
  } else {
    izq = izq * (-1);
    if(izq > 255){
      izq = 255;
    }
    ledcWrite(ledChannel, 0);
    ledcWrite(ledChannel1, izq);
  }
  //motor derecho//
  if (der >= 0) {
    if(der > 255){
      der = 255;
    }
    ledcWrite(ledChannel2, der);
    ledcWrite(ledChannel3, 0);
  } else {
    der = der * (-1);
    if(der > 255){
      der = 255;
    }
    ledcWrite(ledChannel2, 0);
    ledcWrite(ledChannel3, der);
  }
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
  delay(100);
  for (int x = 0; x < 8; x++){
    int valor_prom = 0;
    for (int i = 0; i < 10; i++)
      valor_prom += analogRead(sensores[x]);
    valor_blanco[x] = valor_prom / 10;
  }
  SerialBT.println("[CAL] Lectura BLANCO:");
  printArrayBT("blanco", valor_blanco, 8);

  while (digitalRead(boton) == 1){}
  delay(100);
  while (digitalRead(boton) == 0){} // se usa en 0 por el pull up - seria un 1 logico -
  delay(100);
  for (int x = 0; x < 8; x++){
    int valor_prom = 0;
    for (int i = 0; i < 10; i++)
      valor_prom += analogRead(sensores[x]);
    valor_negro[x] = valor_prom / 10;
  }
  SerialBT.println("[CAL] Lectura NEGRO:");
  printArrayBT("negro", valor_negro, 8);
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
 int valores_digi[SENSORES_TOTAL];
 long suma = 0; // Usar long para evitar desbordamiento en sumas grandes
 long suma_total = 0;

 for (int i = 0; i < SENSORES_TOTAL; i++) {
     int lectura_analogica = analogRead(sensores[i]);

     if (lectura_analogica > valor_umbrales[i]) {
        valores_digi[i] = 1; // Vio la línea
    } else {
        valores_digi[i] = 0; // Vio el fondo
    }
      // Usamos el puntero a los pines miembro _pinesSensores
      // Leemos los pines como digitales (0 o 1). Si fueran analógicos, sería analogRead().
      suma += valores_digi[i];
      suma_total += (long)valores_digi[i] * i * 1000;
    }

if (suma == 0){
      return lastError; // Usar _ultimoError (variable miembro)
    }

    // Posición centrada es: (NUM_SENSORES - 1) * 1000 / 2 = 7 * 1000 / 2 = 3500
    // La posición retornada será (sumaponderada / suma) - 3500
    int posicion = (int)(suma_total / suma);//- ((SENSORES_TOTAL - 1) * 500); 
    return posicion;
  }

void setup(){
  SerialBT.begin(115200);

  SerialBT.begin("vexus");

  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){D1, D2, D3, D4, D5, D6, D7, D8}, SENSORES_TOTAL);

  // declaracion de pines regleta
  /*for (int i = 0; i < 8; i++){
    pinMode(sensores[i], INPUT);
  }*/

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

  //calibrar_sensores();

  // analogRead() takes about 0.1 ms on an AVR.
  // 0.1 ms per sensor * 4 samples per sensor read (default) * 6 sensors
  // * 10 reads per calibrate() call = ~24 ms per calibrate() call.
  // Call calibrate() 400 times to make calibration take about 10 seconds.
  
  while(digitalRead(boton)){}

  digitalWrite(led, HIGH); // LED ON: inicio calibración
  
  for (uint16_t i = 0; i < 400; i++){
    qtr.calibrate();
  }

  digitalWrite(led, LOW);

  while (!esperando_inicio){
    if (digitalRead(boton) == LOW){
      tiempo_ini = millis(); // CRÍTICO: Asignar el tiempo de inicio UNA SOLA VEZ
      esperando_inicio = true;
    }
    return;
  }

  while(!activo){
    precaucion();
  }


  /*
  if (esperando_inicio){
    precaucion();
    return;
  }*/

}

void loop(){

    // desde 0 a 5000 (para leer linea blanca usar readLineWhite() en vez de readLineBlack())
    uint16_t posicion = qtr.readLineBlack(sensorValues);
    //int posicion = leer_linea();

    error = setpoint - posicion;

    derivative = error - lastError;
    //integral += error;
    lastError = error;
    outputPID = (kp * error) + (kd * derivative);

    int velocidad_izq = VBASE - outputPID;
    int velocidad_der = VBASE +  outputPID;

    //motores(velocidad_izq, velocidad_der);
      // Envía los valores de los 8 sensores separados por comas
  /*for (int i = 0; i < SENSORES_TOTAL; i++) {
    SerialBT.print("Sensor n°");
    SerialBT.print(i);
    SerialBT.print("= ");
    SerialBT.print(analogRead(sensores[i]));
    if (i < SENSORES_TOTAL - 1) SerialBT.print(",");
  
    SerialBT.println(); // Salto de línea para la siguiente lectura
    delay(100);
  }

  SerialBT.print("Posicion = ");
  SerialBT.println(posicion);
*/
  for (uint8_t i = 0; i < SENSORES_TOTAL; i++){
    SerialBT.print("Sensor n°");
    SerialBT.print(i);
    SerialBT.print("= ");
    SerialBT.print(sensorValues[i]);
    SerialBT.print('\t');
  }
  SerialBT.print("Posicion = ");
  SerialBT.println(posicion);

  SerialBT.print("Error: ");
  SerialBT.print(error);
  SerialBT.print("Output: ");
  SerialBT.print(outputPID);


  delay(300);

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
